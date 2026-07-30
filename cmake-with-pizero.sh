#!/bin/bash
#

mkdir -p ./build
cmake -S . -B build     -DCMAKE_BUILD_TYPE=Release     -DENABLE_PYCSPOT=OFF
cmake --build build --parallel 1

