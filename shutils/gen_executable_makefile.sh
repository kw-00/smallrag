#!/bin/bash



USAGE=$(cat << EOF
Usage:
BUILD_DIR='absolute_path_to_build_dir' \
    CC='compiler_program' \
    AR='archive_program' \
    [ CFLAGS='cflags_as_from_pkg-config' ] \
    [ LIBS='libs_as_from_pkg-config' ] \
    $? 

Run from module root. include/ and src/ folders are expected in module.
EOF
)

if [ -z "$BUILD_DIR" ]; then
    echo "BUILD_DIR required" >&2
    exit 1
fi
if [ -z "$CC" ]; then
    echo "CC required" >&2
    exit 1
fi

MODULE_NAME=$(basename "$(pwd -P)")

RELATIVE_BUILD_DIR=$(relpath.sh "$(pwd -P)" "$BUILD_DIR")

if [ -d include ]; then
    headers=$(find include -type f -regex '.*\.h$' -exec printf '%s ' {} \;)
else
    headers=
fi
sources="$@"

if [ -z "$sources" ]; then
    echo "Expected at least one .c source file as an argument" >&2
    exit 1
fi

temp=tmp
mkdir "$temp"
sources_dir="$temp/sources"
executables_dir="$temp/executables"
mkdir "$sources_dir" "$executables_dir"
i=0
executables=
for source in $sources; do
    echo "$source" > "$sources_dir/$i"
    executable="$RELATIVE_BUILD_DIR/modules/$MODULE_NAME/${source%??}"
    echo "$executable" > "$executables_dir/$i"
    executables+="$executable "
    i=$((i + 1))
done
source_count=$i
CFLAGS="$CFLAGS $(pkg-config --cflags --static "$MODULE_NAME")"
LIBS="$LIBS $(pkg-config --libs --static "$MODULE_NAME")"
dependency_include_stamps=$(pkg-config \
    --cflags "$MODULE_NAME" \
    | tr ' ' '\n' | grep '^-I.*include$' \
    | sed 's/^-I//g' \
    | sed 's/$/\/\.stamp/g' \
    | sed 's|'"$BUILD_DIR/$MODULE_NAME/include/\.stamp||g" \
    | grep -s '[^\s]' \
    | xargs -r -d '\n' -n 1 bash -c 'printf '"'"'%s '"'"' "$(relpath.sh "$(pwd -P)" $1)"' _)

echo "all: $executables" 
echo
i=0
while [ $i -lt $source_count ]; do
    source=$(cat "$sources_dir/$i")
    executable=$(cat "$executables_dir/$i")
    echo "$executable: $source $headers $dependency_include_stamps"
    echo -e "\tmkdir -p $(dirname "$executable")"
    echo -e "\t$CC -Iinclude $CFLAGS $LIBS -o \$@ $source"
    i=$((i + 1))
done
echo

rm -rf "$temp"

