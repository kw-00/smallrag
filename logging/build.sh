
export BUILD_DIR=$(relpath "$(pwd -P)" "$BUILD_DIR")
export MODULE_NAME=logging

MODULE_BUILD_DIR="$BUILD_DIR/$MODULE_NAME"

mkdir -p "$MODULE_BUILD_DIR/lib"
mkdir -p "$MODULE_BUILD_DIR/include"
mkdir -p "$MODULE_BUILD_DIR/obj"

sed \
    -e 's|@PREFIX@|'"$MODULE_BUILD_DIR"'|g' \
    logging.pc.in \
    > "$BUILD_DIR/pkgconfig/$MODULE_NAME.pc"

TARGET_TYPE="a" gen_makefile > Makefile.generated

"$MAKE" -f Makefile.generated
