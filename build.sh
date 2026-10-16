
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


temp="tmp"
mkdir "$temp"
targets="$temp/targets.txt"
touch "$targets"
recipes="$temp/recipes.txt"
touch "$recipes"

grep ':' ModuleRelationships | while IFS= read -r entry; do
    i=0
    target=
    prerequisites=
    for token in $entry; do
        if [ $i -eq 0 ]; then
            target="${token%:}"
        else
            prerequisites+="$token "
        fi
        i=$((i + 1))
    done
    target_stamp="$relative_build_dir/$target/.stamp"
    printf '%s ' "$target_stamp" >> "$targets"

    printf "$target_stamp:" >> "$recipes"
    for prerequisite in $prerequisites; do
        printf ' %s' "$relative_build_dir/$prerequisite/.stamp" >> "$recipes"
    done
    echo >> "$recipes"
    echo -e "\tcd $target && ./build.sh" >> "$recipes"
    echo -e "\ttouch $target_stamp" >> "$recipes"
    echo >> "$recipes"
done

echo "all: $(cat "$targets")" > Makefile.generated
echo >> Makefile.generated
cat "$recipes" >> Makefile.generated

rm -rf "$temp"
"$MAKE" -f Makefile.generated
