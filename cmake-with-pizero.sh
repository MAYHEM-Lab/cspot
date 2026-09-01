#!/bin/bash
#

cd /home/rich/cspot
mkdir -p ./build
mkdir -p _install
cmake -S . -B build     -DCMAKE_BUILD_TYPE=Debug     -DENABLE_PYCSPOT=OFF -DCMAKE_INSTALL_PREFIX=$PWD/_install
cmake --build build --parallel 1

