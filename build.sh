
export PATH="$PATH:$(pwd -P)/shutils"

export MAKE="make"
export CC="gcc"
export AR="ar"
export BUILD_DIR="$(pwd -P)/build"
export PKG_CONFIG_PATH="$BUILD_DIR/pkgconfig"
export C_STD_FLAG="-std=gnu11"
export CFLAGS="$C_STD_FLAG -Wall -Wextra -Werror"
export PICFLAGS="-fPIC"
export VISIBILITY_HIDDEN_FLAG="-fvisibility=hidden"

mkdir -p "$BUILD_DIR/pkgconfig"

"$MAKE" -f Makefile

