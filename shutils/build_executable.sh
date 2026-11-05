#!/bin/bash

set -e

if [ -z "$BUILD_DIR" ]; then
    echo "BUILD_DIR required" >&2
    exit 1
fi

MODULE_NAME=$(basename "$(pwd -P)")
MODULE_DIR="$BUILD_DIR/modules/$MODULE_NAME"
mkdir -p "$MODULE_DIR"

cp "$MODULE_NAME.pc" "$BUILD_DIR/pkgconfig/$MODULE_NAME.pc"

if [ -z "$TARGET_DIR" ]; then
    export TARGET_DIR="$BUILD_DIR/bin"
fi

gen_executable_makefile.sh $@ > Makefile.generated

"$MAKE" -f Makefile.generated

