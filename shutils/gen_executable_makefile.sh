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
if [ -z "$TARGET_DIR" ]; then
    echo "TARGET_DIR required" >&2
    exit 1
fi
if [ -z "$CC" ]; then
    echo "CC required" >&2
    exit 1
fi


MODULE_NAME=$(basename "$(pwd -P)")

RELATIVE_TARGET_DIR=$(relpath.sh "$(pwd -P)" "$TARGET_DIR")

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
    executable="$RELATIVE_TARGET_DIR/${source%??}"
    echo "$executable" > "$executables_dir/$i"
    executables+="$executable "
    i=$((i + 1))
done
source_count=$i
if ! CFLAGS="$CFLAGS $(pkg-config --cflags --static "$MODULE_NAME")"; then
    echo "pkg-config failed; coult not extract cflags for $MODULE_NAME" >&2
    exit 1
fi
if ! LIBS="$LIBS $(pkg-config --libs --static "$MODULE_NAME")"; then
    echo "pkg-config failed; coult not extract libs for $MODULE_NAME" >&2
    exit 1
fi

dependency_include_stamps=$(pkg-config \
    --cflags "$MODULE_NAME" \
    | tr ' ' '\n' | grep '^-I.*include$' \
    | sed 's/^-I//g' \
    | sed 's/$/\/\.stamp/g' \
    | sed 's|'"$BUILD_DIR/$MODULE_NAME/include/\.stamp||g" \
    | grep -s '[^\s]' \
    | xargs -r -d '\n' -n 1 bash -c 'printf '"'"'%s '"'"' "$(relpath.sh "$(pwd -P)" $1)"' _)

if [ ! $? -eq 0 ]; then
    echo "Failed while determining dependency include stamps" >&2
    exit 1
fi

echo "all: $executables" 
echo
i=0
while [ $i -lt $source_count ]; do
    source=$(cat "$sources_dir/$i")
    executable=$(cat "$executables_dir/$i")
    echo "$executable: $source $headers $dependency_include_stamps"
    echo -e "\tmkdir -p $(dirname "$executable")"
    echo -e "\t$CC $source -Iinclude $CFLAGS $LIBS -o \$@"
    i=$((i + 1))
done
echo

rm -rf "$temp"

