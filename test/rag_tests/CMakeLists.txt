

add_executable(smallrag_text_tests
    smallrag_text_tests.c
)

add_test(NAME smallrag_text_tests COMMAND smallrag_text_tests)

target_link_libraries(smallrag_text_tests PRIVATE
    smallrag_rag
    smallrag_logging
    smallrag_err
)


