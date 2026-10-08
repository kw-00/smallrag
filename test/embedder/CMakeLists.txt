include(FetchLlama)

add_compile_definitions(MODEL_PATH="${CMAKE_SOURCE_DIR}/models/v5-nano-retrieval-F16.gguf")

# llama embedder
add_executable(smallrag_test_llama_embedder
    llama_embedder.c
)

target_link_libraries(smallrag_test_llama_embedder PRIVATE
    smallrag_embedder
    llama
)
use_llama(smallrag_test_llama_embedder)

add_test(NAME smallrag/embedder/llama_embedder COMMAND smallrag_test_llama_embedder)
