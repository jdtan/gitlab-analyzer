#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hh"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <Options.hh>
#include "DataConverter.cc"
#include "DataGraph.hh"
#include "Graph.hh"
#include "PatternMatching.hh"


namespace std
{
    template<class T>
    ostream& operator<<(ostream& stream, const std::pair<T, T> in) {
        return stream << "(" << in.first << ", " << in.second << ")";
    }

    ostream& operator<<(ostream& stream, const vector<uint32_t>& in) {
        stream << "[";
        for(size_t i = 0; i < in.size(); ++i)
            if(i < in.size() - 1)
                stream << in[i] << ", ";
            else
                stream << in[i];
        stream << "]";
        return stream;
    }

    ostream& operator<<(ostream& stream, const vector<vector<uint32_t>>& in) {
        stream << "[";
        for(size_t i = 0; i < in.size(); ++i)
            if(i < in.size() - 1)
                stream << in[i] << ", ";
            else
                stream << in[i];
        stream << "]";
        return stream;
    }

    template<class T>
    ostream& operator<<(ostream& stream, const vector<std::pair<T, T>>& in) {
        stream << "[";
        for(size_t i = 0; i < in.size(); ++i)
            if(i < in.size() - 1)
                stream << in[i] << ", ";
            else
                stream << in[i];
        stream << "]";
        return stream;
    }
}

using namespace std;

bool isTwoListsContainSameElement(const vector<uint32_t>& orgVertexList, const vector<uint32_t>& trueVertexList) {
    if (orgVertexList.size() != trueVertexList.size())
        return false;
    for (auto vertex : trueVertexList) {
        if(!(find(orgVertexList.begin(), orgVertexList.end(), vertex) != orgVertexList.end()))
            return false;
    }
    return true;
}

void testSmallGraphCase1(const Peregrine::SmallGraph& tempGraph)  {
    SUBCASE("Testing SmallGraph1") {
        CHECK(tempGraph.num_vertices() == 4);
        CHECK(tempGraph.v_list() == vector<uint32_t> {1,2,3,4});
        CHECK(isTwoListsContainSameElement(tempGraph.get_out_neighbours(1), vector<uint32_t>{2, 3, 4}) == true);
        CHECK(tempGraph.num_true_edges_in() == 5);
    }
}

void testSmallGraphCase2(const Peregrine::SmallGraph& tempGraph)  {
    SUBCASE("Testing SmallGraph2") {
        CHECK(tempGraph.num_vertices() == 4);
        CHECK(tempGraph.v_list() == vector<uint32_t> {1,2,3,4});
        CHECK(isTwoListsContainSameElement(tempGraph.get_out_neighbours(1), vector<uint32_t>{2, 3, 4}) == true);
        CHECK(tempGraph.num_true_edges_in() == 6);
    }
}

void testSmallGraphCase3(const Peregrine::SmallGraph& tempGraph)  {
    SUBCASE("Testing SmallGraph3") {
        CHECK(tempGraph.num_vertices() == 4);
        CHECK(tempGraph.v_list() == vector<uint32_t> {1,2,3,4});
        CHECK(isTwoListsContainSameElement(tempGraph.get_out_neighbours(1), vector<uint32_t>{4}) == true);
        CHECK(tempGraph.num_true_edges_in() == 6);
    }
}

void testSmallGraphCase4(const Peregrine::SmallGraph& tempGraph)  {
    SUBCASE("Testing SmallGraph4") {
        CHECK(tempGraph.num_vertices() == 6);
        CHECK(tempGraph.v_list() == vector<uint32_t> {1,2,3,4,5,6});
        CHECK(tempGraph.get_out_neighbours(1) == vector<uint32_t> {2,3,6});
        CHECK(tempGraph.num_true_edges_in() == 8);
    }
}

void testAnalyzedPatternCase1(const Peregrine::AnalyzedPattern& analyzedPattern)  {
    SUBCASE("Testing AnalyzedPattern1") {
        CHECK(analyzedPattern.conditions == vector<pair<uint32_t, uint32_t>> {pair(2, 4)});
        CHECK(analyzedPattern.order_groups == vector<vector<uint32_t>> {{2,4}});
    }
}

void testAnalyzedPatternCase2(const Peregrine::AnalyzedPattern& analyzedPattern)  {
    SUBCASE("Testing AnalyzedPattern2") {
        CHECK(analyzedPattern.conditions == vector<pair<uint32_t, uint32_t>> {pair(1, 3), pair(2, 4)});
        CHECK(analyzedPattern.order_groups == vector<vector<uint32_t>> {{2,4}});
    }
}

void testAnalyzedPatternCase3(const Peregrine::AnalyzedPattern& analyzedPattern)  {
    SUBCASE("Testing AnalyzedPattern3") {
        CHECK(analyzedPattern.conditions == vector<pair<uint32_t, uint32_t>> {});
        CHECK(analyzedPattern.order_groups == vector<vector<uint32_t>> {{2},{4}});
    }
}

void testAnalyzedPatternCase4(const Peregrine::AnalyzedPattern& analyzedPattern)  {
    SUBCASE("Testing AnalyzedPattern4") {
        CHECK(analyzedPattern.conditions == vector<pair<uint32_t, uint32_t>> {pair(1, 2)});
        CHECK(analyzedPattern.order_groups == vector<vector<uint32_t>> {{5},{6}});
    }
}

TEST_CASE("GraphTest") {
    std::string fileList[] = {"data/TestCase1.txt", "data/TestCase2.txt", "data/TestCase3.txt", "data/TestCase4.txt"};

    for (const auto& fileName : fileList) {
        Peregrine::SmallGraph tempGraph = Peregrine::SmallGraph(fileName);

        if (fileName == "data/TestCase1.txt") {
            testSmallGraphCase1(tempGraph);
            testAnalyzedPatternCase1(Peregrine::AnalyzedPattern(tempGraph));
        }
        else if (fileName == "data/TestCase2.txt") {
            testSmallGraphCase2(tempGraph);
            testAnalyzedPatternCase2(Peregrine::AnalyzedPattern(tempGraph));
        }
        else if (fileName == "data/TestCase3.txt") {
            testSmallGraphCase3(tempGraph);
            testAnalyzedPatternCase3(Peregrine::AnalyzedPattern(tempGraph));
        }
        else if (fileName == "data/TestCase4.txt") {
            testSmallGraphCase4(tempGraph);
            testAnalyzedPatternCase4(Peregrine::AnalyzedPattern(tempGraph));
        }

        SUBCASE("Check anti edges and vertices") {
            CHECK(tempGraph.num_anti_edges_in() == 0);
            CHECK(tempGraph.num_anti_vertices() == 0);
        }
    }
}

TEST_CASE("DataConverterTest") {
    using namespace Peregrine::DataConverter;

    SUBCASE("TestCase1") {
        convert_data("data/TestCase1.txt", "", "data/output");

        Peregrine::DataGraph graph = Peregrine::DataGraph("data/output");
        CHECK(graph.get_vertex_count() == 4);
        CHECK(graph.get_edge_count() == 5);
        CHECK(graph.original_id(0) == 1);
        CHECK(graph.original_id(1) == 2);

        Peregrine::adjlist myAdjList = graph.get_adj_out(1);
        CHECK(myAdjList.length == 3);
    }

    SUBCASE("Wiki-Vote") {
        convert_data("data/Wiki-Vote.txt", "", "data/output");

        Peregrine::DataGraph graph = Peregrine::DataGraph("data/output");
        CHECK(graph.get_vertex_count() == 7115);
        CHECK(graph.get_edge_count() == 103689);
    }
}

TEST_CASE("PatternMatching") {
    using namespace Peregrine;

    SmallGraph dataGraphAsSmallGraph = SmallGraph("data/SmallTestDataGraph.txt");
    DataGraph dataGraph(dataGraphAsSmallGraph);
    SmallGraph pattern = SmallGraph("data/SmallTestPattern.txt");

    SUBCASE("map_into") {
        std::vector<std::vector<uint32_t>> cands(pattern.num_vertices() + 2, std::vector<uint32_t>{});

        uint64_t count = 0;
        uint32_t vgsCount = dataGraph.get_vgs_count();
        uint32_t num_vertices = dataGraph.get_vertex_count();

        const auto process = [&count](const CompleteMatch &) -> void { count += 1; };

        for (uint32_t vgsi = 0; vgsi < vgsCount; ++vgsi) {
            Matcher<false, UNSTOPPABLE, decltype(process)> matcher(dataGraph.rbi, &dataGraph, vgsi, cands, process);
            for (uint32_t v = 1; v <= num_vertices; ++v) {
                matcher.map_into<Graph::UNLABELLED, false>(v);
            }
        }
    }
}