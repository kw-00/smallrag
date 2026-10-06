macro(fetch_llama target_name)
    include(FetchContent)    
    # Configure llama.cpp before adding it
    set(LLAMA_BUILD_TESTS OFF)
    set(LLAMA_BUILD_EXAMPLES OFF)
    set(LLAMA_BUILD_TOOLS OFF)
    set(LLAMA_BUILD_SERVER OFF)
    set(LLAMA_BUILD_COMMON OFF)

    # TODO - remove this - they exist only because I happen to be using HIP
    set(GGML_HIP ON CACHE BOOL "" FORCE)
    set(GPU_TARGETS gfx1100 CACHE STRING "" FORCE)

    FetchContent_Declare(
        llama
        GIT_REPOSITORY https://github.com/ggml-org/llama.cpp.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(${target_name})
endmacro()
