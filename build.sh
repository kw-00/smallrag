#!/bin/bash

source build_var_source.sh || exit 1

mkdir -p "$BUILD_DIR/pkgconfig"

embedder_configuration=$(printf '%s\n' "$BUILD_BACKENDS" "$GGUF_MODEL_PATHS" "$ONNX_MODEL_PATH")
configuration_file="$BUILD_DIR/.embedder_configuration"
if [ ! -f "$configuration_file" ] || [ "$(cat "$configuration_file")" != "$embedder_configuration" ]; then
    # Backend macros must not reuse objects or test executables from a different build type.
    rm -f \
        "$BUILD_DIR/modules/embedder/obj/src/llama_embedder.o" \
        "$BUILD_DIR/modules/embedder/obj/src/onnx_embedder.o" \
        "$BUILD_DIR/modules/embedder/lib/libembedder.a" \
        "$BUILD_DIR/bin/smallrag" \
        "$BUILD_DIR/test/embedder" \
        "$BUILD_DIR/test/llama_embedder" \
        "$BUILD_DIR/test/llama_embedder_params"
    printf '%s\n' "$embedder_configuration" > "$configuration_file"
fi

module_enabled() {
    case "$1" in
        llama.cpp) [ "$BUILD_LLAMA" = true ] ;;
        onnxruntime|hf_tokenizer) [ "$BUILD_ONNX" = true ] ;;
        *) return 0 ;;
    esac
}

relative_build_dir=$(relpath.sh "$(pwd -P)" "$BUILD_DIR")
relative_build_dir="${relative_build_dir#/}"


temp="tmp"
mkdir "$temp"
modules="$temp/modules.txt"
touch "$modules"
recipes="$temp/recipes.txt"
touch "$recipes"

grep ':' ModuleRelationships | while IFS= read -r entry; do
    i=0
    target=
    prerequisites=
    for token in $entry; do
        if [ $i -eq 0 ]; then
            target="${token%:}"
        elif module_enabled "$token"; then
            prerequisites+="$token "
        fi
        i=$((i + 1))
    done
    if ! module_enabled "$target"; then
        continue
    fi
    target_path=$(find . -type f -regex ".*$target/MODULE_MARKER$" -exec printf '%s ' {} \;)
    i=0
    for path in $target_path; do
        if [ $i -gt 0 ]; then
            echo "Conflict: found two candidate modules for name \"$target\"" >&2
            exit 1
        fi
        target_path=$(dirname "$path")
        i=$((i + 1))
    done

    target_stamp="$relative_build_dir/modules/$target/.stamp"
    printf '%s ' "$target_stamp" >> "$modules"

    printf "$target_stamp:" >> "$recipes"
    for prerequisite in $prerequisites; do
        printf ' %s' "$relative_build_dir/modules/$prerequisite/.stamp" >> "$recipes"
    done
    echo >> "$recipes"
    echo -e "\tcd $target_path && ./build.sh" >> "$recipes"
    echo -e "\ttouch $target_stamp" >> "$recipes"
    echo >> "$recipes"
done

echo "all: $(cat "$modules")" > Makefile.generated
echo >> Makefile.generated
cat "$recipes" >> Makefile.generated

rm -rf "$temp"

find "$BUILD_DIR" -regex '^'"$BUILD_DIR"'/modules/\w+/\.stamp$' -exec rm {} \;
"$MAKE" -f Makefile.generated
