

export BUILD_DIR=$(relpath "$(pwd -P)" "$BUILD_DIR")
export MODULE_NAME=allocation

prepare_build_dirs

TARGET_TYPE="a" gen_makefile > Makefile.generated

"$MAKE" -f Makefile.generated
