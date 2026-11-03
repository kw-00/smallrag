#!/bin/bash

MODULE_NAME=$(basename "$(pwd -P)")
MODULE_BUILD_DIR="$BUILD_DIR/modules/$MODULE_NAME"
mkdir -p "$MODULE_BUILD_DIR"

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
            ln -sfT "$(pwd -P)/$generated" "$MODULE_BUILD_DIR/$(basename "$target")"
            ;;
        *)
            ln -sfT "$(pwd -P)/$script" "$MODULE_BUILD_DIR/$(basename "$script")"
            ;;
    esac
    
done

