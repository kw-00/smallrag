

add_library(smallrag_err
    src/err.c
)

target_include_directories(smallrag_err PUBLIC
    include
)

target_link_libraries(smallrag_err PRIVATE
    smallrag_logging
)
