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
#include <filesystem>

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
        // pair<vertexID, sum of the inDeg and outDeg for that vertex>
        vector<pair<uint32_t, uint32_t>> deg_list = get_sorted_degrees(edge_list);
        auto* vertexMap = new uint32_t[deg_list.size()];
        unordered_map<uint32_t, uint32_t> org_to_new_vertex_map;
        normalize_vertices(vertexMap, deg_list, ref(org_to_new_vertex_map));
        map_org_edge_list_to_normalized(ref(edge_list), org_to_new_vertex_map);

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
        if (threadID == numThreads - 1)
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
            if (graph[cursor-1] != '\n') {
                while (cursor < fileSize && graph[cursor] != '\n')
                    cursor++;
                cursor++;
            }
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
                edgeList.verticesList.insert(element.first);

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
                edgeList.verticesList.insert(element.first);
            }
        }
    }

    vector<pair<uint32_t, uint32_t>> get_sorted_degrees(edge_list_directed &edge_list) {
        vector<pair<uint32_t, uint32_t>> degList;

        // * Sum up the out degrees and in degrees
//        for (auto element : edge_list.verticesList) {
//            auto vertex = element;
//            uint32_t inDeg = 0, outDeg = 0;
//
//            if(edge_list.outEdgeList.find(vertex) != edge_list.outEdgeList.end())
//                outDeg = edge_list.outEdgeList[vertex].size();
//            if(edge_list.inEdgeList.find(vertex) != edge_list.inEdgeList.end())
//                outDeg = edge_list.inEdgeList[vertex].size();
//
//            degList.push_back(pair(vertex, inDeg + outDeg));
//        }

        transform(
                // TODO: Check
//                execution::par,
                edge_list.verticesList.cbegin(), edge_list.verticesList.cend(),
                back_inserter(degList),
                [&edge_list](const uint32_t &element) -> pair<uint32_t, uint32_t> {
                    uint32_t vertex = element;
                    uint32_t inDeg = 0, outDeg = 0;

                    if(edge_list.outEdgeList.find(vertex) != edge_list.outEdgeList.cend())
                        outDeg = edge_list.outEdgeList[vertex].size();
                    if(edge_list.inEdgeList.find(vertex) != edge_list.inEdgeList.cend())
                        outDeg = edge_list.inEdgeList[vertex].size();

                    return pair(vertex, inDeg + outDeg);
                });

        sort(degList.begin(), degList.end(),
             [](const pair<uint32_t, uint32_t>& first, const pair<uint32_t, uint32_t>& second)
             { return first.second > second.second; });

        return degList;
    }

    void normalize_vertices(uint32_t *vertexMap, 
        const vector<std::pair<uint32_t, uint32_t>> &deg_list, 
        std::unordered_map<uint32_t, uint32_t> &org_to_new_vertex_map) {
        uint32_t count = 0;
        for (auto const& vertex : deg_list) {
            vertexMap[count] = vertex.first;
            org_to_new_vertex_map[vertex.first] = count;
            count++;
        }
    }

    void map_org_edge_list_to_normalized(
        edge_list_directed &edge_list, 
        const std::unordered_map<uint32_t, uint32_t> org_to_new_vertex_map) {
        for(auto &[_, myList] : edge_list.inEdgeList) {
            for(uint32_t i = 0; i < myList.size(); i++){
                myList[i] = org_to_new_vertex_map.at(myList[i]);
            }
        }

        for(auto &[_, myList] : edge_list.outEdgeList) {
            for(uint32_t i = 0; i < myList.size(); i++){
                myList[i] = org_to_new_vertex_map.at(myList[i]);
            }
        }
    }

    void write_graph_to_drive(const std::string &outputDir,
                              edge_list_directed &edge_list,
                              const std::vector<std::pair<uint32_t, uint32_t>> &deg_list) {
        filesystem::create_directory(outputDir + "/temp/out");
        filesystem::create_directory(outputDir + "/temp/in");
        string tempOutputDirOutEdge = outputDir + "/temp/out";
        string tempOutputDirInEdge = outputDir + "/temp/in";

        vector<thread> pool;
        int taskSize = deg_list.size() / numThreads;

        // * edge case when the graph is really small
        if (taskSize <= 0) {
            pool.emplace_back(
                    write_thread_local_files,
                    tempOutputDirOutEdge,
                    ref(edge_list.outEdgeList),
                    ref(deg_list),
                    deg_list.size(),
                    0);

            pool.emplace_back(
                    write_thread_local_files,
                    tempOutputDirInEdge,
                    ref(edge_list.inEdgeList),
                    ref(deg_list),
                    deg_list.size(),
                    0);
        }
        else {
            for (unsigned int i = 0; i < numThreads; i++) {
                pool.emplace_back(
                        write_thread_local_files,
                        tempOutputDirOutEdge,
                        ref(edge_list.outEdgeList),
                        ref(deg_list),
                        taskSize,
                        i);

                pool.emplace_back(
                        write_thread_local_files,
                        tempOutputDirInEdge,
                        ref(edge_list.inEdgeList),
                        ref(deg_list),
                        taskSize,
                        i);
            }
        }
        for (auto &thread : pool)
            thread.join();

        concatenateAllTempFilesForOutEdges(outputDir, deg_list.size(), edge_list.numEdges);
    }

    void write_thread_local_files(const std::string &outputDir,
                                  std::unordered_map<uint32_t, std::vector<uint32_t>> &edge_list,
                                  const std::vector<std::pair<uint32_t, uint32_t>> &deg_list,
                                  unsigned int taskSize,
                                  unsigned int threadID) {
        string filePath = outputDir + to_string(threadID) + ".bin";
        unsigned int startIndex = taskSize * threadID, endIndex;

        if (threadID == numThreads - 1)
            endIndex = deg_list.size();
        else
            endIndex = startIndex + taskSize;

        ofstream output(filePath.c_str(), std::ios::binary | ios::trunc);
        for (auto i = startIndex; i < endIndex; i++) {
            auto element = deg_list.at(i);
            uint32_t numEdges = edge_list[element.first].size();
            output.write(reinterpret_cast<const char *>(&numEdges), sizeof(numEdges));
            output.write(reinterpret_cast<const char *>(
                                 &edge_list[element.first][0]), edge_list[element.first].size() * sizeof(uint32_t));
        }
        output.close();
    }

    void concatenateAllTempFilesForOutEdges(string outputDir, uint32_t numVertices, uint64_t numEdges) {
        string output_path_out = outputDir + "/data_out.bin";
        ofstream output_out(output_path_out.c_str(), std::ios::binary | ios::trunc);

        string output_path_in = outputDir + "/data_in.bin";
        ofstream output_in(output_path_in.c_str(), std::ios::binary | ios::trunc);

        // write total number vertices and edges first
        output_out.write(reinterpret_cast<const char *>(&numVertices), sizeof(numVertices));
        output_out.write(reinterpret_cast<const char *>(&numEdges), sizeof(numEdges));

        // * concatenate the temp files
        for (unsigned  int i = 0; i < numThreads; i++) {
            string thread_local_path_out(outputDir + "/temp/out" + to_string(i) + ".bin");
            string thread_local_path_in(outputDir + "/temp/in" + to_string(i) + ".bin");

            ifstream input_out(thread_local_path_out.c_str(), std::ios::binary);
            output_out << input_out.rdbuf();
            remove(thread_local_path_out.c_str());

            ifstream input_in(thread_local_path_in.c_str(), std::ios::binary);
            output_in << input_in.rdbuf();
            remove(thread_local_path_in.c_str());

            input_in.close();
            input_out.close();
        }
        output_out.close();
        output_in.close();
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
                inputFile.close();
            }
            auto t16 = utils::get_timestamp();
            utils::Log{} << "Converted labels to binary format in " << (t16-t15)/1e6 << "s" << "\n";
        }
    }
}
