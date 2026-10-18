

add_executable(smallrag_test_array
    array.c
)

target_link_libraries(smallrag_test_array PRIVATE
    smallrag_data_structures
)

add_test(NAME smallrag/data_structures/array COMMAND smallrag_test_array)
