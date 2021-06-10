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
        SmallGraph p(pp);
        graph_in_memory_out = std::make_unique<uint32_t[]>(2 * p.num_true_edges());
        data_graph_out = std::make_unique<adjlist[]>(p.num_vertices() + 1);
        graph_in_memory_in = std::make_unique<uint32_t[]>(2 * p.num_true_edges());
        data_graph_in = std::make_unique<adjlist[]>(p.num_vertices() + 1);

        uint32_t cursor = 0;
        for (uint32_t v = 1; v <= p.num_vertices(); ++v) {
            std::sort(p.true_adj_list_out.at(v).begin(), p.true_adj_list_out.at(v).end());

            std::memcpy(&graph_in_memory_out[cursor], &p.true_adj_list_out.at(v)[0], p.true_adj_list_out.at(v).size() * sizeof(uint32_t));
            data_graph_out[v - 1].ptr = &graph_in_memory_out[cursor];
            data_graph_out[v - 1].length = p.true_adj_list_out.at(v).size();
            cursor += p.true_adj_list_out.at(v).size();
        }

        cursor = 0;
        for (uint32_t v = 1; v <= p.num_vertices(); ++v) {
            std::sort(p.true_adj_list_in.at(v).begin(), p.true_adj_list_in.at(v).end());

            std::memcpy(&graph_in_memory_in[cursor], &p.true_adj_list_in.at(v)[0], p.true_adj_list_in.at(v).size() * sizeof(uint32_t));
            data_graph_in[v - 1].ptr = &graph_in_memory_in[cursor];
            data_graph_in[v - 1].length = p.true_adj_list_in.at(v).size();
            cursor += p.true_adj_list_in.at(v).size();
        }

        vertex_count = p.num_vertices();
        edge_count = p.num_true_edges();

        labels = std::make_unique<uint32_t[]>(vertex_count+1);
        if (p.labelling == Graph::LABELLED)
        {
            labelled_graph = true;
            for (uint32_t u = 0; u < p.labels.size(); ++u)
            {
                labels[u+1] = p.labels[u];
            }

            uint32_t min_label = *std::min_element(p.labels.cbegin(), p.labels.cend());
            uint32_t max_label = *std::max_element(p.labels.cbegin(), p.labels.cend());
            label_range = std::make_pair(min_label, max_label);
        }

        ids = std::make_unique<uint32_t[]>(vertex_count+1);
        std::iota(&ids[0], &ids[vertex_count+1], 0);
    }

    DataGraph::DataGraph(const SmallGraph &p) {
        from_smallgraph(p);
    }

    DataGraph::DataGraph(const std::string &path_str) {
        std::filesystem::path data_graph_path(path_str);
        if (std::filesystem::is_directory(data_graph_path))
        {
            read_data_graph_out_edges(path_str);
            read_data_graph_in_edges(path_str);
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
              data_graph_out(std::move(other.data_graph_out)),
              graph_in_memory_out(std::move(other.graph_in_memory_out)),
              known_labels(other.known_labels)
    {
        other.vertex_count = 0;
        other.edge_count = 0;
    }

    void DataGraph::read_data_graph_out_edges(const std::string &path_str) {
        std::filesystem::path data_graph_path(path_str);
        std::filesystem::path data_path(data_graph_path / "data_out.bin");
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
        graph_in_memory_out = std::make_unique<uint32_t[]>(num_data_points - 3);
        data_graph_out = std::make_unique<adjlist[]>(vertex_count);

        input_graph.read(reinterpret_cast<char *>(graph_in_memory_out.get()), (num_data_points - 3) * sizeof(uint32_t));

        uint32_t cursor = 0;
        for (uint32_t i = 0; i < vertex_count; i++)
        {
            data_graph_out[i].length = graph_in_memory_out[cursor];
            data_graph_out[i].ptr = &graph_in_memory_out[++cursor];
            cursor += data_graph_out[i].length;
        }
    }

    void DataGraph::read_data_graph_in_edges(const std::string &path_str) {
        std::filesystem::path data_graph_path(path_str);
        std::filesystem::path data_path(data_graph_path / "data_in.bin");
        if (!std::filesystem::exists(data_path))
        {
            std::cerr << "ERROR: Data graph could not be opened." << std::endl;
            exit(1);
        }

        std::ifstream input_graph(data_path, std::ios::binary);

        // don't count the header (one 32-bit integer and one 64 bit integer)
        uint64_t file_size = std::filesystem::file_size(data_path);
        assert(file_size % 4 == 0);
        uint64_t num_data_points = file_size / 4;
        graph_in_memory_in = std::make_unique<uint32_t[]>(num_data_points);
        data_graph_in = std::make_unique<adjlist[]>(vertex_count);

        input_graph.read(reinterpret_cast<char *>(graph_in_memory_in.get()), num_data_points * sizeof(uint32_t));

        uint32_t cursor = 0;
        for (uint32_t i = 0; i < vertex_count; i++)
        {
            data_graph_in[i].length = graph_in_memory_in[cursor];
            data_graph_in[i].ptr = &graph_in_memory_in[++cursor];
            cursor += data_graph_in[i].length;
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

    const adjlist &DataGraph::get_adj_out(uint32_t v) const
    {
        return data_graph_out[v - 1];
    }

    const adjlist &DataGraph::get_adj_in(uint32_t v) const
    {
        return data_graph_in[v - 1];
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
