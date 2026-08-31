#!/bin/bash
#

mkdir -p ./build
mkdir -p ./install

cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=$PWD/toolchain-musl.cmake \
  -DCMAKE_BUILD_TYPE=Release\
  -DCMAKE_BUILD_TYPE=Debug\
  -DCMAKE_INSTALL_PREFIX=$PWD/install


