add_subdirectory(task)

set(libcornet_sources
    cornet.c
    poller.c)

#include_directories(${CMAKE_SOURCE_DIR}) 
add_library(cornet ${libcornet_sources})
