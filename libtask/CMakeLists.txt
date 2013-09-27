enable_language(C ASM)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -D_CORNET)

#set(CMAKE_ASM_FLAGS "${CFLAGS}")
#set_source_files_properties(foo.s PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")

set(libtask_sources
    asm.S
    context.c
    print.c
    task.c)

add_library(task STATIC ${libtask_sources})
