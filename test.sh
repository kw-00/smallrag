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
find "$BUILD_DIR/modules" -mindepth 2 -maxdepth 2 -regex '.*_tests/.*' -not -name '*.stamp' \( -type f -o -type l -xtype f \) -executable -regex "$regex_arg" \
    -exec bash -c '$1 > /dev/null && echo $1 SUCCESS || echo $1 FAIL' _ {} \;
