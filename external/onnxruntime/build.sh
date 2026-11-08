#!/bin/bash

set -e

if [ "$BUILD_ONNX" = false ]; then
    exit 0
fi

export MODULE_NAME=onnxruntime
TARGET_TYPE="so" prepare_library_build_dirs.sh

if [ -z "$ONNX_PATH" ]; then
    echo "ONNX_PATH required; must point to an ONNX Runtime repository or SDK root" >&2
    exit 1
fi

ONNX_PATH=$(abspath.sh "$ONNX_PATH")
if [ -z "$ONNX_BUILD_PATH" ]; then
    if [ -d "$ONNX_PATH/lib" ]; then
        ONNX_BUILD_PATH="$ONNX_PATH"
    else
        ONNX_BUILD_PATH="$ONNX_PATH/build"
    fi
else
    ONNX_BUILD_PATH=$(abspath.sh "$ONNX_BUILD_PATH")
fi

if [ -f "$ONNX_PATH/include/onnxruntime_c_api.h" ]; then
    ONNX_INCLUDE_DIR="$ONNX_PATH/include"
else
    ONNX_INCLUDE_DIR="$ONNX_PATH/include/onnxruntime/core/session"
fi
if [ ! -f "$ONNX_INCLUDE_DIR/onnxruntime_c_api.h" ]; then
    echo "ONNX Runtime C API headers not found under ONNX_PATH: $ONNX_PATH" >&2
    exit 1
fi

LIB_DIR=
for candidate in "$ONNX_BUILD_PATH/lib" "$ONNX_BUILD_PATH/Linux/Release" "$ONNX_BUILD_PATH"; do
    if [ -f "$candidate/libonnxruntime.so" ]; then
        LIB_DIR="$candidate"
        break
    fi
done
if [ -z "$LIB_DIR" ]; then
    echo "libonnxruntime.so not found under ONNX_BUILD_PATH: $ONNX_BUILD_PATH" >&2
    exit 1
fi

MODULE_BUILD_DIR="$BUILD_DIR/modules/$MODULE_NAME"
cp -p -R "$ONNX_INCLUDE_DIR/." "$MODULE_BUILD_DIR/include"
last_mod_header=
last_mod_time=
while IFS= read -r header; do
    mod_time=$(stat -c '%Y' "$header")
    if [ -z "$last_mod_time" ] || [ "$mod_time" -gt "$last_mod_time" ]; then
        last_mod_time=$mod_time
        last_mod_header=$header
    fi
done < <(find "$ONNX_INCLUDE_DIR" -type f -name '*.h')
touch -r "$last_mod_header" "$MODULE_BUILD_DIR/include/.stamp"

sed \
    -e 's|@LIB_DIR@|'"$LIB_DIR"'|g' \
    -e 's|@INCLUDE_DIR@|'"$MODULE_BUILD_DIR/include"'|g' \
    "$MODULE_NAME.pc.in" \
    > "$BUILD_DIR/pkgconfig/$MODULE_NAME.pc"
