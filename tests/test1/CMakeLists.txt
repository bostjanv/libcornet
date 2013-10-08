include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(test1 test1.c)
target_link_libraries(test1 cornet task)
