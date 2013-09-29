cmake_minimum_required(VERSION 2.8)
project(libcornet)

set(CMAKE_C_FLAGS_DEBUG "-Wall -g")

add_subdirectory(src)
add_subdirectory(test)
