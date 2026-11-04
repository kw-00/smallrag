#!/bin/bash

set -e

build_executable.sh "$@"

MODULE_NAME=$(basename "$(pwd -P)")
touch "$BUILD_DIR/modules/$MODULE_NAME/.test_marker"
