cmake_minimum_required(VERSION 3.12)

project(regular CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(regular main.cpp Types.h)
