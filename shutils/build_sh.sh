#!/bin/bash

MODULE_NAME=$(basename "$(pwd -P)")
MODULE_DIR="$BUILD_DIR/modules/$MODULE_NAME"
mkdir -p "$MODULE_DIR"

if [ -z "$TARGET_DIR" ]; then
    TARGET_DIR="$BUILD_DIR/bin"
fi

for script in "$@"; do
    if [ ! -f "$script" ]; then
        echo "$script is not a file" >&2
        exit 1
    fi
    case "$script" in
        *.in)
            template="$script"
            target="${script%.in}"
            generated="$target.generated"
            sed \
            -e 's|@BUILD_DIR@|'"$BUILD_DIR"'|g' \
            -e 's|@SHUTILS_PATH@|'"$SHUTILS_PATH"'|g' \
            -e 's|@GGUF_MODEL_PATHS@|'"$GGUF_MODEL_PATHS"'|g' \
            "$template" \
            > "$generated"

            chmod +x "$generated"
            ln -sfT "$(pwd -P)/$generated" "$TARGET_DIR/$(basename "$target")"
            ;;
        *)
            ln -sfT "$(pwd -P)/$script" "$TARGET_DIR/$(basename "$script")"
            ;;
    esac
    
done

