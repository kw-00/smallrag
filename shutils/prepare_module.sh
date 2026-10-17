

if [ -z "$TARGET_TYPE" ]; then
    echo "TARGET_TYPE required" >&2
    exit 1
fi

if ! echo "$TARGET_TYPE" | grep 'a\|so\|elf'; then
    echo "TARGET_TYPE must be one of a, so or elf"
    exit 1
fi

prepare_build_dirs.sh


gen_makefile.sh > Makefile.generated

"$MAKE" -f Makefile.generated
