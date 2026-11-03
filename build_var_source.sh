#!/bin/bash

export SHUTILS_PATH="$(pwd -P)/shutils"
export PATH="$PATH:$SHUTILS_PATH"

export MAKE="make"
export CC="gcc"
export AR="ar"
export ROOT_DIR="$(pwd p)"
export BUILD_DIR="$ROOT_DIR/build"
# Models for testing
export GGUF_MODEL_PATHS="$ROOT_DIR/models/v5-nano-retrieval-F16.gguf"
export GGUF_MODEL_PATHS_CFLAGS="-DGGUF_MODEL_PATHS=\"\\\"$GGUF_MODEL_PATHS\\\"\""
export PKG_CONFIG_PATH="$BUILD_DIR/pkgconfig"
export C_STD_FLAG="-std=gnu11"
export CFLAGS="$C_STD_FLAG -Wall -Wextra -Werror"
export PICFLAGS="-fPIC"
export VISIBILITY_HIDDEN_FLAG="-fvisibility=hidden"
