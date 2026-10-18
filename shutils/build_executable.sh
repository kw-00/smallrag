#!/bin/bash

if [ -z "$BUILD_DIR" ]; then
    echo "BUILD_DIR required" >&2
    exit 1
fi

MODULE_NAME=$(basename "$(pwd -P)")

MODULE_BUILD_DIR="$BUILD_DIR/modules/$MODULE_NAME"

mkdir -p "$MODULE_BUILD_DIR"

sed \
    -e 's|@PREFIX@|'"$MODULE_BUILD_DIR"'|g' \
    -e 's|@NAME@|'"$MODULE_NAME"'|g' \
    "$MODULE_NAME.pc.in" \
    > "$BUILD_DIR/pkgconfig/$MODULE_NAME.pc"

gen_executable_makefile.sh $@ > Makefile.generated

"$MAKE" -f Makefile.generated

