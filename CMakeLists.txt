cmake_minimum_required(VERSION 3.20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(_POSIX_C_SOURCE=200800L)

project(smallrag)

add_subdirectory(rag)
add_subdirectory(logging)
add_subdirectory(err)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    enable_testing() 
    add_subdirectory(test)
endif()


