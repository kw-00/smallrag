

add_library(smallrag_logging
    src/logging.c
)

target_include_directories(smallrag_logging PUBLIC
    include
)

