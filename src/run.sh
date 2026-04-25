#!/bin/bash
set -e

# 1. Define and clean the build directory
BUILD_DIR="build"
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning old build directory..."
    rm -rf "$BUILD_DIR"
fi
rm -rf **/___pycache__
find . -name "*.so" -delete

# 2. Configure the project
# -S . specifies the source (current dir), -B build specifies the output dir
cmake -S . -B "$BUILD_DIR"\
    -DPython_EXECUTABLE=$(which python3)    \
    -Dpybind11_DIR=$(python3 -m pybind11 --cmakedir)

# 3. Build the project
# -j $(nproc) uses all available CPU cores for a faster build
cmake --build "$BUILD_DIR" -j $(nproc)

# 4. Run tests
./"$BUILD_DIR"/run_tests

echo "========================================"
echo "Running Python Automation (pytest)"
echo "========================================"

# Run pytest on test_.py
if PYTHONPATH=$(pwd)/"$BUILD_DIR"   pytest test_.py -s; then
    echo "Python tests passed successfully!"
else
    echo "Python tests failed! Aborting build."
    exit 1  # <--- CRITICAL: This tells GitHub Actions the step failed!
fi
rm -rf "$BUILD_DIR"
echo "All builds and tests completed successfully!"
