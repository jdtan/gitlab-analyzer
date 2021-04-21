#include <thread>
#include <atomic>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <execution>
#include <regex>
#include <iostream>
#include <sys/stat.h>
#include "utils.hh"
#include "DataConverter.hh"

// * Only outer edges will be considered
namespace Peregrine::DataConverter {
    using namespace std;

    bool file_exists(const std::string &path)
    {
        struct stat statbuf;
        return (stat(path.c_str(), &statbuf) == 0);
    }

    void convert_data(
            const string &edge_file,
            const string &label_file,
            const string &out_dir) {
        uint64_t numEdges;
        unordered_map<uint32_t, vector<uint32_t>> edge_list;

        read_file_and_generate_graph(edge_file, edge_list, numEdges);
        vector<pair<uint32_t, uint32_t>> deg_list = get_sorted_degrees(edge_list);
        auto* vertexMap = new uint32_t[deg_list.size()];
        normalize_vertices(vertexMap, deg_list);

        write_graph_to_drive(out_dir, edge_list, deg_list, numEdges);
        write_id_map_to_drive(out_dir, vertexMap, deg_list.size());
        write_labels_to_drive(label_file, out_dir, vertexMap);
        delete[] vertexMap;
    }

    void write_labels_to_drive(const string& label_file, const string& out_dir, const uint32_t* ids_rev_map) {
        if (file_exists(label_file))
        {
            auto t15 = utils::get_timestamp();
            {
                std::ifstream ifile(label_file.c_str());

                std::string output_labels(out_dir + "/labels.bin");
                std::ofstream ofile(output_labels.c_str(), std::ios::binary | std::ios::trunc);

                std::string line;
                while (std::getline(ifile, line))
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
                        ofile.write(reinterpret_cast<char *>(&new_u_label[0]), 2*sizeof(uint32_t));
                    }
                }
                ofile.close();
            }
            auto t16 = utils::get_timestamp();
            utils::Log{} << "Converted labels to binary format in " << (t16-t15)/1e6 << "s" << "\n";
        }
    }

    void read_file_and_generate_graph(
            const string &edgeListFile,
            unordered_map<uint32_t, vector<uint32_t>> &edge_list,
            uint64_t &numEdges) {
        ifstream edgeList(edgeListFile);
        string line;
        numEdges = 0;

        while (getline(edgeList, line)) {
            if (line[0] != '#') {
                add_edge_from_line(line, edge_list);
                numEdges++;
            }
        }
    }

    void add_edge_from_line(const string &line, unordered_map<uint32_t, vector<uint32_t>> &edge_list) {
        uint32_t sourceVertex, dstVertex;
        istringstream iss(line);
        iss >> sourceVertex >> dstVertex;
        edge_list[sourceVertex].push_back(dstVertex);
        edge_list[dstVertex];
    }

    vector<pair<uint32_t, uint32_t>> get_sorted_degrees(unordered_map<uint32_t, vector<uint32_t>> &edge_list) {
        vector<pair<uint32_t, uint32_t>> degList;

        for (auto const& element : edge_list ){
            degList.push_back(pair(element.first, element.second.size()));
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
                              std::unordered_map<uint32_t, std::vector<uint32_t>> &edge_list,
                              const std::vector<std::pair<uint32_t, uint32_t>> deg_list,
                              uint64_t numEdges) {
        string output_path = outputDir + "/data.bin";
        ofstream output(output_path.c_str(), std::ios::binary | ios::trunc);

        // write total number vertices and edges first
        uint32_t numVertices = deg_list.size();
        output.write(reinterpret_cast<const char *>(&numVertices), sizeof(numVertices));
        output.write(reinterpret_cast<const char *>(&numEdges), sizeof(numEdges));

        for (auto element : deg_list) {
            output.write(reinterpret_cast<const char *>(&element.second), sizeof(element.second));
            output.write(reinterpret_cast<const char *>(
                                 &edge_list[element.first][0]),edge_list[element.first].size() * sizeof(uint32_t));
        }
        output.close();
    }

    void write_id_map_to_drive(const std::string &outputDir, const uint32_t* vertexMap, const uint32_t size) {
        string output_path = outputDir + "/ids.bin";
        ofstream output(output_path.c_str(), std::ios::binary | ios::trunc);
        output.write(reinterpret_cast<const char *>(vertexMap), size * sizeof(uint32_t));
        output.close();
    }

    void check_if_directed_graph(string &line) {
        string_tolower(line);
        if (line.find("directed") != string::npos)
            throw invalid_argument("This is not a directed graph!\n");
    }

    void check_has_num_edges_and_vertices(const string& line, uint32_t &total_num_vertices, uint32_t &total_num_edges) {
        smatch m;
        regex r("Nodes:[[:space:]]*(\\d+)[[:space:]]*Edges: (\\d+)");

        regex_match(line, m, r);

        if (m.size() != 2)
            throw invalid_argument("Invalid SNAP format\n");

        total_num_edges = stoi(m[0].str());
        total_num_vertices = stoi(m[1].str());
    }

    void string_tolower(string &line) {
        transform(line.begin(), line.end(), line.begin(),
                  [](unsigned char c){ return tolower(c); });
    }
}
