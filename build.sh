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
modules="$temp/modules.txt"
touch "$modules"
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
    target_path=$(find . -type f -regex ".*$target/MODULE_MARKER$" -exec printf '%s ' {} \;)
    i=0
    for path in $target_path; do
        if [ $i -gt 0 ]; then
            echo "Conflict: found two candidate modules for name \"$target\"" >&2
            exit 1
        fi
        target_path=$(dirname "$path")
        i=$((i + 1))
    done

    target_stamp="$relative_build_dir/modules/$target/.stamp"
    printf '%s ' "$target_stamp" >> "$modules"

    printf "$target_stamp:" >> "$recipes"
    for prerequisite in $prerequisites; do
        printf ' %s' "$relative_build_dir/modules/$prerequisite/.stamp" >> "$recipes"
    done
    echo >> "$recipes"
    echo -e "\tcd $target_path && ./build.sh" >> "$recipes"
    echo -e "\ttouch $target_stamp" >> "$recipes"
    echo >> "$recipes"
done

echo "all: $(cat "$modules")" > Makefile.generated
echo >> Makefile.generated
cat "$recipes" >> Makefile.generated

rm -rf "$temp"
"$MAKE" -f Makefile.generated
