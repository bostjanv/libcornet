cmake_minimum_required(VERSION 2.8)
project(libcornet)

set(CMAKE_C_FLAGS_DEBUG "-Wall -g -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS_RELEASE "-Wall -O2 -march=native")

add_subdirectory(src)
add_subdirectory(tests)
