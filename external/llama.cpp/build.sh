

export MODULE_NAME=llama.cpp

TARGET_TYPE="so" prepare_library_build_dirs.sh

if [ -z "$LLAMA_CPP_PATH" ]; then
    echo "LLAMA_CPP_PATH required; must point to a llama.cpp repository root" >&2
    echo "Set LLAMA_CPP_PATH to point to wherever you have llama.cpp installed and built." >&2
    echo "Note: the llama.cpp build directory is expected at \$LLAMA_CPP_PATH/build" >&2
    exit 1
fi

LIBLLAMA_PATH="$LLAMA_CPP_PATH/build/bin/libllama.so"

if [ ! -f "$LIBLLAMA_PATH" ]; then
    echo "Expected llama.cpp shared library at $LIBLLAMA_PATH, but missing" >&2
    exit 1
fi

ln -s "$LIBLLAMA_PATH" "$BUILD_DIR/modules/$MODULE_NAME/lib/libllama.so"
touch -r "$LIBLLAMA_PATH" "$BUILD_DIR/modules/$MODULE_NAME/lib/.stamp"


LLAMA_HEADER_PATH="$LLAMA_CPP_PATH/include"
GGML_HEADER_PATH="$LLAMA_CPP_PATH/ggml/include"

max_timestamp=0
recently_modified_header=
for file in \
    $(find "$LLAMA_HEADER_PATH" -type f -regex '.*\.h') \
    $(find "$GGML_HEADER_PATH" -type f -regex '.*\.h'); do
    timestamp=$(stat -c '%Y' "$file") 
    if [ $timestamp -gt $max_timestamp ]; then
        max_timestamp=$timestamp
        recently_modified_header="$file"
    fi
done
if [ -z "$recently_modified_header" ]; then
    echo "No llama.cpp headers found" >&2
    exit 1
fi

cp -R "$LLAMA_HEADER_PATH/." "$BUILD_DIR/modules/$MODULE_NAME/include"
cp -R "$GGML_HEADER_PATH/." "$BUILD_DIR/modules/$MODULE_NAME/include"

INCLUDE_STAMP="$BUILD_DIR/modules/$MODULE_NAME/include/.stamp"
touch -r "$recently_modified_header" "$INCLUDE_STAMP"

if [ $(stat -c '%Y' "$LLAMA_HEADER_PATH") -gt $(stat -c '%Y' "$GGML_HEADER_PATH") ]; then
    touch -r "$LLAMA_HEADER_PATH" "$INCLUDE_STAMP"
else
    touch -r "$GGML_HEADER_PATH" "$INCLUDE_STAMP"
fi

sed 's|@PREFIX@|'"$BUILD_DIR/modules/$MODULE_NAME"'|g' "$MODULE_NAME.pc.in" > "$BUILD_DIR/pkgconfig/$MODULE_NAME.pc"


