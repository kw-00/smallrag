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
find "$BUILD_DIR/modules" -mindepth 2 -maxdepth 2 -not -name '.test_marker' -not -name '*.stamp' \( -type f -o -type l -xtype f \) -executable -regex "$regex_arg" \
    -exec bash -c '
        if [ -f "$(dirname "$1")/.test_marker" ]; then
            "$1" > /dev/null && echo "$1 SUCCESS" || echo "$1 FAIL"
        fi
    ' _ {} \;
