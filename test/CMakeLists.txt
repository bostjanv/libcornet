include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

add_executable(test1 test1.c)
target_link_libraries(test1 cornet task)

add_executable(test2 test2.c)
target_link_libraries(test2 task)
