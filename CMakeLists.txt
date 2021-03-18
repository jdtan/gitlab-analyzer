cmake_minimum_required(VERSION 3.17)
project(peregrine-directed)

set(CMAKE_CXX_STANDARD 20)

add_executable(
        peregrine-directed
        ./apps/testGraphhh.cc
        ./apps/count.cc
        ./apps/existence-query.cc
        ./apps/fsm.cc)
include_directories(./core)