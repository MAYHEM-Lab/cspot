#!/bin/bash
#

mkdir -p ./build
HERE=`pwd`

#  -DCMAKE_BUILD_TYPE=Debug \
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=$HERE/toolchain-musl.cmake \
  -DCMAKE_BUILD_TYPE=Release



