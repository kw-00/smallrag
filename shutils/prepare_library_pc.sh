#!/bin/bash

if [ -z "$BUILD_DIR" ]; then
    echo "BUILD_DIR required" >&2
    exit 1
fi

if [ -z "$TARGET_TYPE" ]; then
    echo "TARGET_TYPE required" >&2
    exit 1
fi

MODULE_NAME=$(basename "$(pwd -P)")
MODULE_BUILD_DIR="$BUILD_DIR/modules/$MODULE_NAME"

sed \
    -e 's|@PREFIX@|'"$MODULE_BUILD_DIR"'|g' \
    "$MODULE_NAME.pc.in" \
    > "$BUILD_DIR/pkgconfig/$MODULE_NAME.pc"

