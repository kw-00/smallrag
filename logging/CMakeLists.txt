

add_library(logging
    src/logging.c
)

target_include_directories(logging PUBLIC
    include
)

