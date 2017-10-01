set(SOURCES
    src/main.c)

add_executable(mint ${SOURCES})
target_link_libraries(mint mint-lib)

