include(FetchContent)

function(use_c_thread_pool target_name)
    FetchContent_Declare(
        C_Thread_Pool
        GIT_REPOSITORY https://github.com/Pithikos/C-Thread-Pool.git
        GIT_TAG master
    )

    FetchContent_MakeAvailable(C_Thread_Pool)
    FetchContent_GetProperties(C_Thread_Pool)

message(STATUS "SOURCE_DIR = ${C_Thread_Pool_SOURCE_DIR}")
message(STATUS "BINARY_DIR = ${C_Thread_Pool_BINARY_DIR}")
message(STATUS "POPULATED = ${C_Thread_Pool_POPULATED}")
    if (NOT TARGET C_Thread_Pool)
        add_library(C_Thread_Pool STATIC
            "${C_Thread_Pool_SOURCE_DIR}/thpool.c"
        )
        target_include_directories(C_Thread_Pool PUBLIC
            "${C_Thread_Pool_SOURCE_DIR}"
        )
    endif()

    target_link_libraries("${target_name}" PRIVATE
        C_Thread_Pool
    )
endfunction()
