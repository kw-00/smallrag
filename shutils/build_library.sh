

if [ -z "$TARGET_TYPE" ]; then
    echo "TARGET_TYPE required" >&2
    exit 1
fi

if ! echo "$TARGET_TYPE" | grep 'a\|so' > /dev/null; then
    echo "TARGET_TYPE must be one of a or so"
    exit 1
fi

prepare_library_build_dirs.sh
prepare_library_pc.sh

gen_makefile.sh > Makefile.generated

"$MAKE" -f Makefile.generated
