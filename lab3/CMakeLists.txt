cmake_minimum_required(VERSION 3.12)

project(determinator CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(common STATIC Utils.cpp)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(determinator main.cpp)
target_link_libraries(determinator PRIVATE common)

add_executable(trans trans.cpp)
target_link_libraries(trans PRIVATE common)