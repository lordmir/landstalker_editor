#! /bin/bash

mkdir -p build
cmake -S . -B build
cmake --build build --config Release
cmake --install build
