

# char chunker
add_executable(smallrag_test_char_chunker
    char_chunker.c
)

target_link_libraries(smallrag_test_char_chunker PRIVATE
    smallrag_chunker
)

add_test(NAME smallrag/chunking/char_chunker COMMAND smallrag_test_char_chunker)
