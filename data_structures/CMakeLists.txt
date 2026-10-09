



add_library(smallrag_data_structures INTERFACE)

target_include_directories(smallrag_data_structures INTERFACE
    include
)

target_link_libraries(smallrag_data_structures INTERFACE 
    smallrag_logging
)
