cmake_minimum_required(VERSION 3.12)
project(HCP_SAT_CPP CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(.)
include_directories(/usr/include)
link_directories(/usr/lib)

find_package(Threads REQUIRED)

add_executable(test_hcp test_hcp.cpp solver.cpp)
target_link_libraries(test_hcp cadical Threads::Threads)

add_executable(compare_successors compare_successors.cpp solver.cpp)
target_link_libraries(compare_successors cadical Threads::Threads)

add_executable(compare_hcp compare_hcp.cpp solver.cpp)
target_link_libraries(compare_hcp cadical Threads::Threads)
