#!/bin/sh

set -e
umask 077
suffix=$(awk 'BEGIN { srand(); printf "%d", rand() * 1000000000 }')
directory="/tmp/smallrag_${suffix}_$$"
mkdir "$directory"
printf '%s\n' "$directory"
