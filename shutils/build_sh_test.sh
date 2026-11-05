#!/bin/bash

set -e

TARGET_DIR="$BUILD_DIR/test" build_sh.sh "$@"

MODULE_NAME=$(basename "$(pwd -P)")
touch "$BUILD_DIR/modules/$MODULE_NAME/.test_marker"
