

add_library(err
    src/err.c
)

target_include_directories(err PUBLIC
    include
)

target_link_libraries(err PRIVATE
    logging
)
