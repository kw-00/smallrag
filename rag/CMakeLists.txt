include(FetchContent)

# Configure llama.cpp before adding it.
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_COMMON OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    llama
    GIT_REPOSITORY https://github.com/ggml-org/llama.cpp.git
    GIT_TAG master
)

FetchContent_MakeAvailable(llama)

add_library(rag
    src/data_mang.c
)

target_include_directories(rag PUBLIC include)

target_link_libraries(rag PRIVATE 
    llama
    logging
    err
)



