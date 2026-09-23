include(FetchContent)
FetchContent_Declare(
    llama.cpp
    URL https://github.com/ggml-org/llama.cpp.git
    GIT_TAG f95b0d95394d5e311ba8228689972843178c5e28
)
FetchContent_MakeAvailable(llama.cpp)

add_library(embedding
    src/fragments.c
    src/embeddings.c
)

target_include_directories(llama_wrapper PUBLIC include)

target_link_libraries(llama_wrapper PRIVATE llama)
