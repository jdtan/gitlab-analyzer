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

#include "utils.hh"
#include "Graph.hh"
#include "DataGraph.hh"

namespace Peregrine
{
    void DataGraph::from_smallgraph(const SmallGraph &pp) {
    }

    DataGraph::DataGraph(const SmallGraph &p) {
        from_smallgraph(p);
    }

    DataGraph::DataGraph(const std::string &path_str) {
        std::filesystem::path data_graph_path(path_str);
        if (std::filesystem::is_directory(data_graph_path))
        {
            read_data_graph(path_str);
            read_ids_graph(path_str);
            read_labels_graph(path_str);
        }
        else if (std::filesystem::is_regular_file(data_graph_path))
        {
            from_smallgraph(path_str);
        }
        else
        {
            std::cerr << "ERROR: Data graph could not be opened." << std::endl;
            exit(1);
        }
    }

    DataGraph::DataGraph(DataGraph &&other)
            : rbi(other.rbi),
              vertex_count(other.vertex_count),
              edge_count(other.edge_count),
              forest_count(other.forest_count),
              labelled_graph(other.labelled_graph),
              labels(std::move(other.labels)),
              label_range(other.label_range),
              ids(std::move(other.ids)),
              data_graph(std::move(other.data_graph)),
              graph_in_memory(std::move(other.graph_in_memory)),
              known_labels(other.known_labels)
    {
        other.vertex_count = 0;
        other.edge_count = 0;
    }

    void DataGraph::read_data_graph(const std::string &path_str) {
        std::filesystem::path data_graph_path(path_str);
        std::filesystem::path data_path(data_graph_path / "data.bin");
        if (!std::filesystem::exists(data_path))
        {
            std::cerr << "ERROR: Data graph could not be opened." << std::endl;
            exit(1);
        }

        std::ifstream input_graph(data_path, std::ios::binary);

        input_graph.read(reinterpret_cast<char *>(&vertex_count), sizeof(vertex_count));
        input_graph.read(reinterpret_cast<char *>(&edge_count), sizeof(edge_count));

        // don't count the header (one 32-bit integer and one 64 bit integer)
        uint64_t file_size = std::filesystem::file_size(data_path);
        assert(file_size % 4 == 0);
        uint64_t num_data_points = file_size / 4;
        graph_in_memory = std::make_unique<uint32_t[]>(num_data_points - 2);
        data_graph = std::make_unique<adjlist[]>(vertex_count);

        input_graph.read(reinterpret_cast<char *>(graph_in_memory.get()), (num_data_points - 2)*sizeof(uint32_t));

        uint32_t cursor = 0;
        for (uint32_t i = 0; i < vertex_count; i++)
        {
            data_graph[i].length = graph_in_memory[cursor];
            data_graph[i].ptr = &graph_in_memory[++cursor];
            cursor += data_graph[i].length;
        }
    }

    void DataGraph::read_ids_graph(const std::string &path_str) {
        std::filesystem::path data_graph_path(path_str);
        std::filesystem::path id_path(data_graph_path / "ids.bin");
        if (!std::filesystem::exists(id_path))
        {
            std::cerr << "ERROR: ID data could not be opened." << std::endl;
            exit(1);
        }

        std::ifstream input_graph(id_path, std::ios::binary);

        ids = std::make_unique<uint32_t[]>(vertex_count);

        input_graph.read(reinterpret_cast<char *>(ids.get()), vertex_count*sizeof(uint32_t));
    }

    void DataGraph::read_labels_graph(const std::string &path_str) {
        std::filesystem::path data_graph_path(path_str);
        std::filesystem::path label_path(data_graph_path / "labels.bin");
        if (!std::filesystem::exists(label_path)) {
            std::cout << "No label data detected." << std::endl;
            return;
        }

        std::ifstream labels_file(label_path, std::ios::binary);

        uint32_t min_label = UINT_MAX;
        uint32_t max_label = 0;
        labelled_graph = true;

        for (uint32_t i = 0; i < vertex_count; ++i)
        {
            uint32_t buf[2];
            labels_file.read(reinterpret_cast<char *>(buf), 2*sizeof(uint32_t));

            uint32_t v = buf[0];
            uint32_t label = buf[1];
            min_label = std::min(min_label, label);
            max_label = std::max(max_label, label);
            labels[v] = label;
        }

        label_range = {min_label, max_label};
    }

    void DataGraph::set_rbi(const AnalyzedPattern &new_rbi)
    {
        rbi = new_rbi;
        forest_count = rbi.vgs.size();
        if (rbi.labelling_type() == Graph::PARTIALLY_LABELLED)
        {
            new_label = std::distance(rbi.query_graph.labels.cbegin(),
                                      std::find(rbi.query_graph.labels.cbegin(),
                                                rbi.query_graph.labels.cend(), static_cast<uint32_t>(-1))
            );
        }
    }

    void DataGraph::set_known_labels(const std::vector<SmallGraph> &patterns)
    {
        for (const auto &p : patterns)
        {
            known_labels.insert(p.labels.cbegin(), p.labels.cend());
        }
    }

    void DataGraph::set_known_labels(const std::vector<uint32_t> &labels)
    {
        known_labels.insert(labels.cbegin(), labels.cend());
    }

    bool DataGraph::known_label(const uint32_t label) const
    {
        return known_labels.contains(label);
    }

    const std::vector<uint32_t> &DataGraph::get_upper_bounds(uint32_t v) const
    {
        return rbi.upper_bounds[v];
    }

    const std::vector<uint32_t> &DataGraph::get_lower_bounds(uint32_t v) const
    {
        return rbi.lower_bounds[v];
    }

    const std::pair<uint32_t, uint32_t> &DataGraph::get_label_range() const
    {
        return label_range;
    }

    const adjlist &DataGraph::get_adj(uint32_t v) const
    {
        return data_graph[v-1];
    }

    const SmallGraph &DataGraph::get_vgs(unsigned vgsi) const
    {
        return rbi.vgs[vgsi];
    }

    uint32_t DataGraph::original_id(uint32_t v) const
    {
        return ids[v];
    }

    const SmallGraph &DataGraph::get_pattern() const {
        return rbi.query_graph;
    }

    const std::vector<std::vector<uint32_t>> &DataGraph::get_qs(unsigned vgsi) const
    {
        return rbi.qs[vgsi];
    }

    uint32_t DataGraph::label(uint32_t dv) const
    {
        return labels[dv];
    }

    uint32_t DataGraph::vmap_at(unsigned vgsi, uint32_t v, unsigned qsi) const
    {
        return rbi.vmap[vgsi][v][qsi];
    }

    const std::vector<uint32_t> &DataGraph::get_qo(uint32_t vgsi) const
    {
        return rbi.qo_book[vgsi];
    }

    uint32_t DataGraph::get_vgs_count() const
    {
        return rbi.vgs.size();
    }

    uint32_t DataGraph::get_vertex_count() const
    {
        return vertex_count;
    }

    uint64_t DataGraph::get_edge_count() const
    {
        return edge_count;
    }
}
