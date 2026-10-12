


add_library(smallrag_main STATIC
    src/data_loading.c
)

target_include_directories(smallrag_main PUBLIC
    include
)

target_link_libraries(smallrag_main PUBLIC 
    smallrag_chunker
    smallrag_embedder
    smallrag_logging
)
