include(UseLlama)

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

add_executable(smallrag
    programs/smallrag.c
)

target_link_libraries(smallrag PRIVATE 
    smallrag_main
    smallrag_vectors
)
use_llama(smallrag)
