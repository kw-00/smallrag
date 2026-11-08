#!/bin/bash

export SHUTILS_PATH="$(pwd -P)/shutils"
export PATH="$PATH:$SHUTILS_PATH"

export MAKE="make"
export CC="gcc"
export AR="ar"
export ROOT_DIR="$(pwd p)"
export BUILD_DIR="$ROOT_DIR/build"

export BUILD_LLAMA="${BUILD_LLAMA:-true}"
export BUILD_ONNX="${BUILD_ONNX:-true}"
for variable in BUILD_LLAMA BUILD_ONNX; do
    case "${!variable}" in
        true|false) ;;
        *) echo "$variable must be true or false" >&2; return 1 ;;
    esac
done
if [ "$BUILD_LLAMA" = false ] && [ "$BUILD_ONNX" = false ]; then
    echo "At least one of BUILD_LLAMA and BUILD_ONNX must be true" >&2
    return 1
fi

llama_define=0
onnx_define=0
export BUILD_BACKENDS=
export BACKEND_DEPENDENCIES=
if [ "$BUILD_LLAMA" = true ]; then
    llama_define=1
    BUILD_BACKENDS=llama
    BACKEND_DEPENDENCIES=llama.cpp
fi
if [ "$BUILD_ONNX" = true ]; then
    onnx_define=1
    BUILD_BACKENDS="${BUILD_BACKENDS:+$BUILD_BACKENDS,}onnx"
    BACKEND_DEPENDENCIES="${BACKEND_DEPENDENCIES:+$BACKEND_DEPENDENCIES, }onnxruntime, hf_tokenizer"
fi
export BUILD_BACKENDS_CFLAGS="-DBUILD_BACKENDS=\"\\\"$BUILD_BACKENDS\\\"\""

# Models for testing
export GGUF_MODEL_PATHS="$ROOT_DIR/models/v5-nano-retrieval-F16.gguf"
export GGUF_MODEL_PATHS_CFLAGS="-DGGUF_MODEL_PATHS=\"\\\"$GGUF_MODEL_PATHS\\\"\""
export ONNX_MODEL_PATH="${ONNX_MODEL_PATH:-$ROOT_DIR/models/embeddinggemma-2-ONNX}"
export ONNX_MODEL_PATH_CFLAGS="-DONNX_MODEL_PATH=\"\\\"$ONNX_MODEL_PATH\\\"\""
export PKG_CONFIG_PATH="$BUILD_DIR/pkgconfig"
export C_STD_FLAG="-std=gnu11"
export CFLAGS="$C_STD_FLAG -Wall -Wextra -Werror $EXTRA_CFLAGS -DBUILD_LLAMA=$llama_define -DBUILD_ONNX=$onnx_define $BUILD_BACKENDS_CFLAGS"
export PICFLAGS="-fPIC"
export VISIBILITY_HIDDEN_FLAG="-fvisibility=hidden"
