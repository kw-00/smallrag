path="$1"

case "$path" in
    /*) echo "$path"
        ;;
    *) echo "$(pwd -P)/$path"
        ;;
esac
