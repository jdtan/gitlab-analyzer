#ifndef DATA_CONVERTER_HH
#define DATA_CONVERTER_HH

#include <string>
#include <unordered_set>

namespace Peregrine::DataConverter
{
    struct edge_list_directed {
        uint64_t numEdges = 0;
        std::unordered_set<uint32_t> verticesList;

        std::unordered_map<uint32_t, std::vector<uint32_t>> inEdgeList;
        std::unordered_map<uint32_t, std::vector<uint32_t>> outEdgeList;
    };

    void convert_data(const std::string &edge_file,
                      const std::string &label_file,
                      const std::string &out_dir);

    void read_file_and_generate_graph(const std::string &edgeListFile,
                                      edge_list_directed &edgeList);

    void populateGraph (unsigned int threadID,
                        unsigned int chunkSize,
                        std::unordered_map<uint32_t, std::vector<uint32_t>> &inEdgeList,
                        std::unordered_map<uint32_t, std::vector<uint32_t>> &outEdgeList,
                        const char *graph,
                        size_t fileSize);

    void combine_edge_list(std::vector<std::unordered_map<uint32_t, std::vector<uint32_t>>> &inEdgeList,
                           std::vector<std::unordered_map<uint32_t, std::vector<uint32_t>>> &outEdgeList,
                           edge_list_directed &edgeList);

    std::vector<std::pair<uint32_t, uint32_t>> get_sorted_degrees(edge_list_directed &edge_list);

    void normalize_vertices(uint32_t *vertexMap, const std::vector<std::pair<uint32_t, uint32_t>> &deg_list);

    void write_graph_to_drive(const std::string &outputDir,
                              edge_list_directed &edge_list,
                              const std::vector<std::pair<uint32_t, uint32_t>> &deg_list);

    void write_thread_local_files(const std::string &outputDir,
                                  std::unordered_map<uint32_t, std::vector<uint32_t>> &edge_list,
                                  const std::vector<std::pair<uint32_t, uint32_t>> &deg_list,
                                  unsigned int taskSize,
                                  unsigned int threadID);

    void concatenateAllTempFilesForOutEdges(std::string outputDir, uint32_t numVertices, uint64_t numEdges);

    void write_id_map_to_drive(const std::string &outputDir, const uint32_t* vertexMap, const uint32_t &size);

    void write_labels_to_drive(const std::string& label_file, const std::string& out_dir, const uint32_t* ids_rev_map);
}

#endif
