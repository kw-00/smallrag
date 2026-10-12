

add_library(smallrag_vectors STATIC
    src/normalization.c
    src/dot_product.c
)

target_include_directories(smallrag_vectors PUBLIC
    include
)

target_link_libraries(smallrag_vectors PRIVATE
    smallrag_logging
    m
)

