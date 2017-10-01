project(mint)

cmake_minimum_required(VERSION 3.1)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(SOURCES
    src/linkedlist.c
    src/lexer.c
    src/parser.c
    src/codegen.c
    src/compiler.c
    src/symbols.c
    src/dict.c
    src/hash.c
    src/typer.c
    src/macro.c
    src/utils.c
    src/vm.c)

add_library(mint-lib STATIC ${SOURCES})
target_include_directories(mint-lib PUBLIC include)

add_subdirectory(runner)
