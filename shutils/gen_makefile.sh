#!/bin/bash



USAGE=$(cat << EOF
Usage:
BUILD_DIR='absolute_path_to_build_dir' \
    TARGET_TYPE='a_so_or_elf' \
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
if [ -z "$TARGET_TYPE" ]; then
    echo "TARGET_TYPE required" >&2
    exit 1
fi
if [ -z "$CC" ]; then
    echo "CC required" >&2
    exit 1
fi
if [ -z "$AR" ]; then
    echo "AR required" >&2
    exit 1
fi

MODULE_NAME=$(basename "$(pwd -P)")

RELATIVE_BUILD_DIR=$(relpath.sh "$(pwd -P)" "$BUILD_DIR")

is_a=false
is_so=false
is_elf=false
if [ "$TARGET_TYPE" = "a" ]; then
    is_a=true
elif [ "$TARGET_TYPE" = "so" ]; then
    is_so=true
elif [ "$TARGET_TYPE" = "elf" ]; then
    is_elf=true
else
    echo "TARGET_TYPE must be one of a, so and elf" >&2
    exit 1
fi


headers=$(find include -type f -regex '.*\.h$' -exec printf '%s ' {} \;)
sources=$(find src -type f -regex '.*\.c$' -exec printf '%s ' {} \;) 

if [ -z "$sources" ]; then
    echo "Expected at least one .c source file in src/"
    exit 1
fi
if [ -z "$headers" ]; then
    echo "Expected at least one .h header file in include/"
    exit 1
fi

temp=tmp
mkdir "$temp"
sources_dir="$temp/sources"
objects_dir="$temp/objects"
mkdir "$sources_dir" "$objects_dir"
i=0
objects=
for source in $sources; do
    echo "$source" > "$sources_dir/$i"
    object="$BUILD_DIR/$MODULE_NAME/obj/${source%??}.o"
    echo "$object" > "$objects_dir/$i"
    objects+="$object "
    i=$((i + 1))
done
source_count=$i
if $is_elf; then
    target_path="$RELATIVE_BUILD_DIR/$MODULE_NAME/executable/$MODULE_NAME.$TARGET_TYPE"
else
    target_path="$RELATIVE_BUILD_DIR/$MODULE_NAME/lib/lib$MODULE_NAME.$TARGET_TYPE"
fi
CFLAGS="$CFLAGS $(pkg-config --cflags "$MODULE_NAME")"
LIBS="$LIBS $(pkg-config --libs "$MODULE_NAME")"
dependency_include_stamps=$(pkg-config \
    --cflags "$MODULE_NAME" \
    | tr ' ' '\n' | grep '^-I.*include$' \
    | sed 's/^-I//g' \
    | sed 's/$/\/\.stamp/g' \
    | sed 's|'"$BUILD_DIR/$MODULE_NAME/include/\.stamp||g" \
    | grep -s '[^\s]' \
    | xargs -r -d '\n' -n 1 bash -c 'printf '"'"'%s '"'"' "$(relpath.sh "$(pwd -P)" $1)"' _)

echo "all: $RELATIVE_BUILD_DIR/$MODULE_NAME/lib/.stamp $RELATIVE_BUILD_DIR/$MODULE_NAME/include/.stamp"
echo
echo "$RELATIVE_BUILD_DIR/$MODULE_NAME/lib/.stamp: $target_path"
echo -e "\ttouch \$@"
echo
echo "$target_path: $objects"
if $is_a; then
    echo -e "\t$AR rcs \$@ \$^"
elif $is_so; then
    echo -e "\t$CC -shared -o \$@ \$^ $LIBS"
else
    # Executable
    echo -e "\t$CC -o \$@ \$^ $LIBS"
fi
echo
i=0
while [ $i -lt $source_count ]; do
    source=$(cat "$sources_dir/$i")
    object=$(cat "$objects_dir/$i")
    echo "$object: $source $headers $dependency_include_stamps"
    echo -e "\tmkdir -p $(dirname "$object")"
    if $is_so; then
        echo -e "\t$CC -c -Iinclude $CFLAGS $PICFLAGS -o \$@ $source"
    elif $is_a; then
        echo -e "\t$CC -c $VISIBILITY_HIDDEN_FLAG -Iinclude $CFLAGS -o \$@ $source"
	else
        echo -e "\t$CC -c -Iinclude $CFLAGS -o \$@ $source"
    fi
    i=$((i + 1))
done
echo
echo "$RELATIVE_BUILD_DIR/$MODULE_NAME/include/.stamp: $headers"
echo -e "\tcp -R include/. $RELATIVE_BUILD_DIR/$MODULE_NAME/include"
echo -e "\ttouch \$@"
echo

rm -rf "$temp"

