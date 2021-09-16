#include <iostream>
#include "Peregrine.hh"
#include "DataConverter.cc"

using namespace std;

int main(int argc, char *argv[]) {
    Peregrine::DataConverter::convert_data(argv[1], "", "data/output");
    string pattern_name = "data/SmallTestPattern.txt";
    string data_graph_name = "data/output";

    int numThreads = 1;
    vector<Peregrine::SmallGraph> pattern = {Peregrine::SmallGraph(pattern_name)};

    const auto process = [](auto &&aggregatorHandle, auto &&completeMatch)
    {
        aggregatorHandle.map(completeMatch.pattern, 1);
    };

    // Peregrine::DataGraph graph = Peregrine::DataGraph("data/output");
    // for (uint32_t u = 1; u <= 7115; u++)
    // {
    //     Peregrine::adjlist adj_in = graph.get_adj_in(u);
    //     for (uint32_t i = 0; i < adj_in.length; i++)
    //     {
    //         if (adj_in.ptr[i] > 7115) {
    //             std::cout << "adj_in list is wrong: " << adj_in.ptr[i] << "?\n";
    //             exit(-1);
    //         }
    //     }

    //     Peregrine::adjlist adj_out = graph.get_adj_out(u);
    //     for (uint32_t i = 0; i < adj_out.length; i++)
    //     {
    //         if (adj_out.ptr[i] > 7115) {
    //             std::cout << "adj_out list is wrong: " << adj_out.ptr[i] << "?\n";
    //             exit(-1);
    //         }
    //     }
    // }

    vector<pair<Peregrine::SmallGraph, uint32_t>> results =
        Peregrine::match<Peregrine::Pattern, uint32_t, Peregrine::AT_THE_END, Peregrine::UNSTOPPABLE>(
            data_graph_name, pattern, numThreads, process);

    int patternCount = 0;
    for (auto &[pattern, count] : results)
    {
        cout << patternCount++ << ": " << count << endl;
    }
}