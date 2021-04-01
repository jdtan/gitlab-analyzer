#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hh"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include "Graph.hh"

using namespace std;

void print_vector(const vector<uint32_t>& myVector) {
    cout << "{ ";
    for (auto & i : myVector)
        std::cout << i << ' ';
    cout << "}\n";
}

void print_vector(const vector<vector<uint32_t>>& myVector) {
    cout << "{ ";
    for (const auto& i : myVector) {
        cout << "{ ";
        for (auto j : i)
            std::cout << j << ' ';
        cout << "} ";
    }
    cout << "}\n";
}

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
        print_vector(analyzedPattern.order_groups);
        CHECK(analyzedPattern.order_groups == vector<vector<uint32_t>> {{2},{4}});
    }
}

void testAnalyzedPatternCase4(const Peregrine::AnalyzedPattern& analyzedPattern)  {
    SUBCASE("Testing AnalyzedPattern4") {
        CHECK(analyzedPattern.conditions == vector<pair<uint32_t, uint32_t>> {pair(1, 2)});
        print_vector(analyzedPattern.order_groups);
        CHECK(analyzedPattern.order_groups == vector<vector<uint32_t>> {{5},{6}});
    }
}

TEST_CASE("Test") {
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
