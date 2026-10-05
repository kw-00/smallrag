

add_library(smallrag_chunker STATIC
    src/chunker_wrapper_functions.c
    src/char_chunker.c
)

target_include_directories(smallrag_chunker PUBLIC
    include
)

target_link_libraries(smallrag_chunker PRIVATE
    smallrag_logging
)
