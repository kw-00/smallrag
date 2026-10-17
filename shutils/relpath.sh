#!/bin/sh
# POSIX relative path, text only, same kind absolute/relative

src="$1"
target="$2"

[ "$src" = "$target" ] && { printf ""; exit 0; }

common="$src"
up=""

while :; do
  prefix=${target#"$common"}
  if [ "$prefix" != "$target" ]; then
    case "$prefix" in
      "" ) break ;;
      /* ) break ;;
      * )
        case "$common" in
          */ ) break ;;
        esac
        ;;
    esac
  fi

  case "$common" in
    "/"|"."|"") common=""; break ;;
  esac

  if [ -z "$up" ]; then
    up=".."
  else
    up="$up/.."
  fi
  common=$(dirname "$common")
done

if [ -z "$common" ]; then
  forward="$target"
else
  forward=${target#"$common"}
  case "$common" in
    */ ) : ;;
    * ) case "$forward" in /*) forward=${forward#/} ;; esac ;;
  esac
fi

if [ -z "$up" ]; then
  printf "%s" "$forward"
else
  if [ -n "$forward" ]; then
    printf "%s" "$up/$forward"
  else
    printf "%s" "$up"
  fi
fi

