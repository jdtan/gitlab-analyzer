cmake_minimum_required(VERSION 3.17)
project(peregrine_directed_clion)

set(CMAKE_CXX_STANDARD 20)

add_executable(
        peregrine_directed_clion
        ./apps/testGraphhh.cc
        ./apps/count.cc
        ./apps/existence-query.cc
        ./apps/fsm.cc)
include_directories(./core)