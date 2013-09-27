set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -D_CORNET)

set(libtask_sources
    asm.S
    context.c
    print.c
    task.c)

add_library(task STATIC ${libtask_sources})
