

export MODULE_NAME=llama.cpp

TARGET_TYPE="so" prepare_library_build_dirs.sh

if [ -z "$LLAMA_CPP_PATH" ]; then
    echo "LLAMA_CPP_PATH required; must point to a llama.cpp repository root" >&2
    echo "Set LLAMA_CPP_PATH to point to wherever you have llama.cpp installed and built." >&2
    echo "Note: the llama.cpp build directory is expected at \$LLAMA_CPP_PATH/build" >&2
    exit 1
fi

LLAMA_CPP_PATH=$(abspath.sh "$LLAMA_CPP_PATH")
echo "$LLAMA_CPP_PATH ====="

LIB_DIR="$LLAMA_CPP_PATH/build/bin"
LLAMA_INCLUDE_DIR="$LLAMA_CPP_PATH/include"
GGML_INCLUDE_DIR="$LLAMA_CPP_PATH/ggml/include"

MODULE_BUILD_DIR="$BUILD_DIR/modules/$MODULE_NAME"
cp -p -R "$LLAMA_INCLUDE_DIR/." "$MODULE_BUILD_DIR/include"
cp -p -R "$GGML_INCLUDE_DIR/." "$MODULE_BUILD_DIR/include"

last_mod_header=
last_mod_time=
for header in \
    $(find "$LLAMA_INCLUDE_DIR" -type f -name '*.h') \
    $(find "$GGML_INCLUDE_DIR" -type f -name '*.h'); do
    mod_time=$(stat -c '%Y' "$header")
    if [ -z $last_mod_time ] || [ $mod_time -gt $last_mod_time ]; then
        last_mod_time=$mod_time
        last_mod_header="$header"
    fi
done

if [ -z "$last_mod_header" ]; then
    echo "No headers found, unexpectedly" >&2
    exit 1
fi

touch -r "$last_mod_header" "$MODULE_BUILD_DIR/include/.stamp"

sed \
    -e 's|@LIB_DIR@|'"$LIB_DIR"'|g' \
    -e 's|@INCLUDE_DIR@|'"$MODULE_BUILD_DIR/include"'|g' \
    "$MODULE_NAME.pc.in" \
    > "$BUILD_DIR/pkgconfig/$MODULE_NAME.pc"


