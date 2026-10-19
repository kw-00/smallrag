
CJSON_H_URL="https://raw.githubusercontent.com/DaveGamble/cJSON/refs/heads/master/cJSON.h"
CJSON_C_URL="https://raw.githubusercontent.com/DaveGamble/cJSON/refs/heads/master/cJSON.c"

prepare_library_build_dirs.sh

MODULE_NAME=$(basename "$(pwd -P)")

INCLUDE_DIR="$BUILD_DIR/targets/$MODULE_NAME/include"
if [ ! -d "$INCLUDE_DIR" ]; then
    mkdir -p "$INCLUDE_DIR"
fi
if [ ! -f "$INCLUDE_DIR/.stamp" ]; then
    curl -o "$INCLUDE_DIR/cJSON.h" "$CJSON_H_URL"
    touch "$INCLUDE_DIR/.stamp"
fi

LIB_DIR="$BUILD_DIR/targets/$MODULE_NAME/lib"
if [ ! -d "$LIB_DIR" ]; then
    mkdir -p "$LIB_DIR"
fi
if [ ! -f "$LIB_DIR/.stamp" ]; then
    curl -O "$CJSON_C_URL"
    "$CC" -c -I"$INCLUDE_DIR" -o cJSON.o cJSON.c \
    && "$AR" rcs "$LIB_DIR/libcJSON.a" cJSON.o \
    && touch "$LIB_DIR/.stamp"
    rm cJSON.o cJSON.c
fi

sed -i 's|@PREFIX@|'"$BUILD_DIR/targets/$MODULE_NAME"'|g' "$MODULE_NAME.pc.in" \
    > "$BUILD_DIR/pkgconfig/$MODULE_NAME.pc"


