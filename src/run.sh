#!/bin/bash
set -e

# 1. Define and clean the build directory
BUILD_DIR="build"
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning old build directory..."
    rm -rf "$BUILD_DIR"
fi

# 2. Configure the project
# -S . specifies the source (current dir), -B build specifies the output dir
cmake -S . -B "$BUILD_DIR"

# 3. Build the project
# -j $(nproc) uses all available CPU cores for a faster build
cmake --build "$BUILD_DIR" -j $(nproc)

# 4. Run tests
./"$BUILD_DIR"/run_tests

