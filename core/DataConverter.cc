#include <thread>
#include <atomic>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <execution>

#include <cassert>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <regex>

#include "utils.hh"
#include "DataConverter.hh"

// * Only outer edges will be considered
namespace Peregrine::DataConverter {
    using namespace std;
    const unsigned numThreads = std::thread::hardware_concurrency();

    bool file_exists(const std::string &path)
    {
        struct stat statbuf;
        return (stat(path.c_str(), &statbuf) == 0);
    }

    void convert_data(
            const string &edge_file,
            const string &label_file,
            const string &out_dir) {
        edge_list_directed edge_list;

        read_file_and_generate_graph(edge_file, edge_list);
        vector<pair<uint32_t, uint32_t>> deg_list = get_sorted_degrees(edge_list);
        auto* vertexMap = new uint32_t[deg_list.size()];
        normalize_vertices(vertexMap, deg_list);

        write_graph_to_drive(out_dir, edge_list, deg_list);
        write_id_map_to_drive(out_dir, vertexMap, deg_list.size());
        write_labels_to_drive(label_file, out_dir, vertexMap);
        delete[] vertexMap;
    }

    void read_file_and_generate_graph(
            const string &edgeListFile,
            edge_list_directed &edgeList) {
        vector<unordered_map<uint32_t, vector<uint32_t>>> inEdgeList(numThreads);
        vector<unordered_map<uint32_t, vector<uint32_t>>> outEdgeList(numThreads);

        struct stat edge_file_stat;
        int edge_fd = open(edgeListFile.c_str(), O_RDONLY, 0);
        assert(edge_fd != -1);
        assert(fstat(edge_fd, &edge_file_stat) == 0);

        size_t edge_file_size = edge_file_stat.st_size;
        char *graph_data = static_cast<char*>(mmap(NULL,
                                                   edge_file_size,
                                                   PROT_READ,
                                                   MAP_PRIVATE | MAP_POPULATE,
                                                   edge_fd,
                                                   0));

        vector<thread> pool;
        size_t taskSize = edge_file_size / numThreads;

        if (taskSize <= 4)
            pool.emplace_back(populateGraph,
                              0,
                              edge_file_size,
                              ref(inEdgeList[0]),
                              ref(outEdgeList[0]),
                              graph_data,
                              edge_file_size);
        else {
            for (unsigned int i = 0; i < numThreads; i++)
                pool.emplace_back(populateGraph,
                                  i,
                                  taskSize,
                                  ref(inEdgeList[i]),
                                  ref(outEdgeList[i]),
                                  graph_data,
                                  edge_file_size);
        }
        for (auto &thread : pool)
            thread.join();

        combine_edge_list(inEdgeList, outEdgeList, edgeList);
    }

    void populateGraph (unsigned int threadID,
                        unsigned int chunkSize,
                        unordered_map<uint32_t, vector<uint32_t>> &inEdgeList,
                        unordered_map<uint32_t, vector<uint32_t>> &outEdgeList,
                        const char *graph,
                        size_t fileSize) {
        size_t end, start = threadID * chunkSize;
        if (threadID <= numThreads - 1)
            end = fileSize;
        else
            end = start + chunkSize;

        uint32_t source, destination;
        size_t cursor;
        // * Determine the initial cursor position
        if (threadID == 0)
            cursor = 0;
        else {
            cursor = start;
            while (cursor < fileSize && graph[cursor] != '\n')
                cursor++;
            cursor++;
        }

        while (cursor < end) {
            // find the end of this edge
            size_t line_end = cursor;
            while (line_end < fileSize && graph[line_end] != '\n')
                line_end++;
            if (line_end >= fileSize)
                break;

            // read the vertices from it
            std::string line(&graph[cursor], &graph[line_end]);

            cursor = line_end+1;

            if (line[0] == '#')
                continue;

            std::istringstream iss(line);
            iss >> source >> destination;
            outEdgeList[source].push_back(destination);
            inEdgeList[destination].push_back(source);
        }
    }

    void combine_edge_list(vector<unordered_map<uint32_t, vector<uint32_t>>> &inEdgeList,
                           vector<unordered_map<uint32_t, vector<uint32_t>>> &outEdgeList,
                           edge_list_directed &edgeList) {
        for (unsigned int i = 0; i < inEdgeList.size(); i++) {
            for (auto &element : inEdgeList[i]) {
                edgeList.inEdgeList[element.first].insert(
                        edgeList.inEdgeList[element.first].begin(),
                        element.second.begin(),
                        element.second.end());
                edgeList.verticesList[element.first];

                // * construct edge size at the same time
                edgeList.numEdges += element.second.size();
            }
        }

        for (unsigned int i = 0; i < outEdgeList.size(); i++) {
            for (auto &element : outEdgeList[i]) {
                edgeList.outEdgeList[element.first].insert(
                        edgeList.outEdgeList[element.first].begin(),
                        element.second.begin(),
                        element.second.end());
                edgeList.verticesList[element.first];
            }
        }
    }

    vector<pair<uint32_t, uint32_t>> get_sorted_degrees(edge_list_directed &edge_list) {
        vector<pair<uint32_t, uint32_t>> degList;

        // ! transform_reduce
        // * Sum up the out degrees and in degrees
        for (auto element : edge_list.verticesList) {
            auto vertex = element.first;
            uint32_t inDeg = 0, outDeg = 0;

            if(edge_list.outEdgeList.find(vertex) != edge_list.outEdgeList.end())
                outDeg = edge_list.outEdgeList[vertex].size();
            if(edge_list.inEdgeList.find(vertex) != edge_list.inEdgeList.end())
                outDeg = edge_list.inEdgeList[vertex].size();

            degList.push_back(pair(vertex, inDeg + outDeg));
        }

        sort(degList.begin(), degList.end(),
             [](const pair<uint32_t, uint32_t>& first, const pair<uint32_t, uint32_t>& second)
             { return first.second > second.second; });

        return degList;
    }

    void normalize_vertices(uint32_t *vertexMap, const vector<std::pair<uint32_t, uint32_t>> &deg_list) {
        uint32_t count = 0;
        for (auto const& vertex : deg_list) {
            vertexMap[count] = vertex.first;
            count++;
        }
    }

    void write_graph_to_drive(const std::string &outputDir,
                              edge_list_directed &edge_list,
                              const std::vector<std::pair<uint32_t, uint32_t>> &deg_list) {
        vector<thread> pool;
        int taskSize = deg_list.size() / numThreads;

        // * edge case when the graph is really small
        if (taskSize <= 0)
            pool.emplace_back(
                    write_thread_local_files,
                    outputDir,
                    ref(edge_list.outEdgeList),
                    ref(deg_list),
                    deg_list.size(),
                    0);
        else {
            for (unsigned int i = 0; i < numThreads; i++)
                pool.emplace_back(
                        write_thread_local_files,
                        outputDir,
                        ref(edge_list.outEdgeList),
                        deg_list,
                        taskSize,
                        i);
        }
        for (auto &thread : pool)
            thread.join();

        concatenate_all_temp_files(outputDir, deg_list.size(), edge_list.numEdges);
    }

    void write_thread_local_files(const std::string &outputDir,
                                  std::unordered_map<uint32_t, std::vector<uint32_t>> &edge_list,
                                  const std::vector<std::pair<uint32_t, uint32_t>> &deg_list,
                                  unsigned int taskSize,
                                  unsigned int threadID) {
        string filePath = outputDir + "/temp/" + to_string(threadID) + ".bin";
        unsigned int startIndex = taskSize * threadID, endIndex;

        if (threadID == numThreads - 1)
            endIndex = deg_list.size();
        else
            endIndex = startIndex + taskSize;

        ofstream output(filePath.c_str(), std::ios::binary | ios::trunc);
        for (auto i = startIndex; i < endIndex; i++) {
            auto element = deg_list.at(i);
            output.write(reinterpret_cast<const char *>(&element.second), sizeof(element.second));
            output.write(reinterpret_cast<const char *>(
                                 &edge_list[element.first][0]),edge_list[element.first].size() * sizeof(uint32_t));
        }
    }

    void concatenate_all_temp_files(string outputDir, uint32_t numVertices, uint64_t numEdges) {
        string output_path = outputDir + "/data.bin";
        ofstream output(output_path.c_str(), std::ios::binary | ios::trunc);

        // write total number vertices and edges first
        output.write(reinterpret_cast<const char *>(&numVertices), sizeof(numVertices));
        output.write(reinterpret_cast<const char *>(&numEdges), sizeof(numEdges));

        // * concatenate the temp files
        for (unsigned  int i = 0; i < numThreads; i++) {
            string thread_local_path(outputDir + "/temp/" + to_string(i) + ".bin");

            ifstream input(thread_local_path.c_str(), std::ios::binary);
            output << input.rdbuf();
            remove(thread_local_path.c_str());
        }
        output.close();
    }

    void write_id_map_to_drive(const std::string &outputDir, const uint32_t* vertexMap, const uint32_t &size) {
        string output_path = outputDir + "/ids.bin";
        ofstream output(output_path.c_str(), std::ios::binary | ios::trunc);
        output.write(reinterpret_cast<const char *>(vertexMap), size * sizeof(uint32_t));
        output.close();
    }

    void write_labels_to_drive(const string& label_file, const string& out_dir, const uint32_t* ids_rev_map) {
        if (file_exists(label_file))
        {
            auto t15 = utils::get_timestamp();
            {
                std::ifstream inputFile(label_file.c_str());

                std::string output_labels(out_dir + "/labels.bin");
                std::ofstream outputFile(output_labels.c_str(), std::ios::binary | std::ios::trunc);

                std::string line;
                while (std::getline(inputFile, line))
                {
                    // easy to predict properly
                    if (line[0] == '#') continue;
                    std::istringstream iss(line);

                    uint32_t u;
                    uint32_t new_u_label[2];
                    iss >> u >> new_u_label[1];
                    new_u_label[0] = ids_rev_map[u]+1;

                    if (new_u_label[0] != 0) // ids_rev_map[u] != -1
                    {
                        outputFile.write(reinterpret_cast<char *>(&new_u_label[0]), 2 * sizeof(uint32_t));
                    }
                }
                outputFile.close();
            }
            auto t16 = utils::get_timestamp();
            utils::Log{} << "Converted labels to binary format in " << (t16-t15)/1e6 << "s" << "\n";
        }
    }
}
