
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

relative_build_dir=$(relpath.sh "$(pwd -P)" "$BUILD_DIR")
relative_build_dir="${relative_build_dir#/}"

sed 's|@STAMP>\([-a-zA-Z0-9_/]\+\)@|'"$relative_build_dir/\\1/.stamp"'|g' Makefile.in > Makefile.generated

stamps=$(grep -o '.*\.stamp:' Makefile.generated | sed 's/.$//g' | tr '\n' ' ')

sed -i 's|@STAMPS@|'"$stamps"'|g' Makefile.generated

"$MAKE" -f Makefile.generated

