

export BUILD_DIR=$(relpath.sh "$(pwd -P)" "$BUILD_DIR")
export MODULE_NAME=chunker

prepare_build_dirs.sh

TARGET_TYPE="a" gen_makefile.sh > Makefile.generated

"$MAKE" -f Makefile.generated
