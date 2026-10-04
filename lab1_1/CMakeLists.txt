cmake_minimum_required(VERSION 3.16)
project(Automata CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(automata_common STATIC "${CMAKE_CURRENT_SOURCE_DIR}/Utils.cpp")
target_include_directories(automata_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(converter "${CMAKE_CURRENT_SOURCE_DIR}/main_convert.cpp")
target_link_libraries(converter PRIVATE automata_common)

add_executable(minimizer "${CMAKE_CURRENT_SOURCE_DIR}/main_minimize.cpp")
target_link_libraries(minimizer PRIVATE automata_common)
