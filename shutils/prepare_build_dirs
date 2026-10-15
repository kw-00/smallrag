#!/bin/bash


MODULE_BUILD_DIR="$BUILD_DIR/$MODULE_NAME"

mkdir -p "$MODULE_BUILD_DIR/lib"
mkdir -p "$MODULE_BUILD_DIR/include"
mkdir -p "$MODULE_BUILD_DIR/obj"

sed \
    -e 's|@PREFIX@|'"$MODULE_BUILD_DIR"'|g' \
    "$MODULE_NAME.pc.in" \
    > "$BUILD_DIR/pkgconfig/$MODULE_NAME.pc"

