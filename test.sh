#!/bin/bash

source build_var_source.sh

if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory does not exist. Build the project before testing" >&2
    exit 1
fi

if [ -n "$1" ]; then
    regex_arg="$1"
else
    regex_arg='.*'
fi

find "$BUILD_DIR/test" -mindepth 1 -maxdepth 1 -type f -executable -regex "$regex_arg" \
    -exec bash -c '"$1" > /dev/null 2>&1 && echo "$1 SUCCESS" || echo "$1 FAIL"' _ {} \;

