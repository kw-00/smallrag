#!/bin/bash

if [ -z "$BUILD_DIR" ]; then
    echo "BUILD_DIR required" >&2
    exit 1
fi

MODULE_NAME=$(basename "$(pwd -P)")
MODULE_BUILD_DIR="$BUILD_DIR/modules/$MODULE_NAME"

mkdir -p "$MODULE_BUILD_DIR/lib"
mkdir -p "$MODULE_BUILD_DIR/include"
mkdir -p "$MODULE_BUILD_DIR/obj"

