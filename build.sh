
export PATH="$PATH:$(pwd -P)/shutils"

export MAKE="${MAKE:-make}"
export CC="${CC:-gcc}"
export AR="${AR:-ar}"
export BUILD_DIR="$(pwd -P)/${BUILD_DIR:-build}"

mkdir -p "$BUILD_DIR/pkgconfig"

"$MAKE" -f Makefile

