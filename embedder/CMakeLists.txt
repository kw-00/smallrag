include(FetchLlama)
fetch_llama(llama)

add_library(smallrag_embedder STATIC
    src/embedder_wrapper_functions.c
    src/llama_embedder.c
)

target_include_directories(smallrag_embedder PUBLIC
    include
)

target_link_libraries(smallrag_embedder PRIVATE
    llama
    smallrag_logging
    smallrag_allocation
)
