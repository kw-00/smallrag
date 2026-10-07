

add_library(smallrag_allocation STATIC
    src/allocation.c
)

target_include_directories(smallrag_allocation PUBLIC
    include
)

target_link_libraries(smallrag_allocation PRIVATE
    smallrag_logging
)
