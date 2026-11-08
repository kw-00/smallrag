#!/bin/bash

set -e

if [ "$BUILD_ONNX" = false ]; then
    exit 0
fi

export TARGET_TYPE="a"
prepare_library_build_dirs.sh
prepare_library_pc.sh

MODULE_BUILD_DIR="$BUILD_DIR/modules/hf_tokenizer"
"${CARGO:-cargo}" build --release --locked --target-dir "$MODULE_BUILD_DIR/cargo"
cp -p "$MODULE_BUILD_DIR/cargo/release/libhf_tokenizer.a" "$MODULE_BUILD_DIR/lib"
cp -p -R include/. "$MODULE_BUILD_DIR/include"
touch -r include/smallrag/hf_tokenizer.h "$MODULE_BUILD_DIR/include/.stamp"
touch -r "$MODULE_BUILD_DIR/lib/libhf_tokenizer.a" "$MODULE_BUILD_DIR/lib/.stamp"
