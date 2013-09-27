set(libcornet_sources
    cornet.c)

include_directories(${CMAKE_SOURCE_DIR}) 
add_library(cornet ${libcornet_sources})
