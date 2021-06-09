cmake_minimum_required(VERSION 3.17)
project(peregrine-directed)

set(CMAKE_CXX_STANDARD 20)

add_executable(
        peregrine-directed
        ./core/doctest.hh
        ./core/Graph.hh
        ./core/DataGraph.cc
        ./core/DataGraph.hh
        ./core/DataConverter.hh
        ./core/DataConverter.cc
        ./core/PatternGenerator.hh
        ./core/PatternGenerator.cc
        ./core/PatternMatching.hh
        ./core/utils.hh
        ./core/utils.cc
        ./core/bliss-0.73/bliss.cc
        ./apps/testGraphhh.cc
        ./apps/count.cc
        ./apps/existence-query.cc
        ./apps/fsm.cc)
include_directories(./core)