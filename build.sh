
export PATH="$PATH:$(pwd -P)/shutils"

export MAKE="${MAKE:-make}"
export CC="${CC:-gcc}"
export AR="${AR:-ar}"
export BUILD_DIR="$(pwd -P)/${BUILD_DIR:-build}"
export PKG_CONFIG_PATH="$BUILD_DIR/pkgconfig"
export CFLAGS="-Wall -Wextra -Werror -std=gnu11"

mkdir -p "$BUILD_DIR/pkgconfig"

"$MAKE" -f Makefile

