#!/bin/bash
#

mkdir -p ./build
mkdir -p ./_install

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release\
  -DCMAKE_BUILD_TYPE=Debug\
  -DCMAKE_INSTALL_PREFIX=$PWD/_install


