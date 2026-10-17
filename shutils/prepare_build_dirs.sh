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

MODULE_BUILD_DIR="$BUILD_DIR/$MODULE_NAME"
if ! echo "$TARGET_TYPE" | grep -s 'a\|so\|elf' > /dev/null; then
    echo "TARGET_TYPE must be one of a, so or elf"
    exit 1
fi
if [ "$TARGET_TYPE" = elf ]; then
    mkdir -p "$MODULE_BUILD_DIR/executable"
else
    mkdir -p "$MODULE_BUILD_DIR/lib"
fi
mkdir -p "$MODULE_BUILD_DIR/include"
mkdir -p "$MODULE_BUILD_DIR/obj"

sed \
    -e 's|@PREFIX@|'"$MODULE_BUILD_DIR"'|g' \
    -e 's|@NAME@|'"$MODULE_NAME"'|g' \
    "$MODULE_NAME.pc.in" \
    > "$BUILD_DIR/pkgconfig/$MODULE_NAME.pc"

