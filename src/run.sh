#!bin/bash
rm -rf build
cmake -S . -B build
cmake --build build -j1
./build/run_tests