#!/bin/bash
#

mkdir -p ./build
HERE=`pwd`

cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=$HERE/toolchain-musl.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_BUILD_TYPE=Debug



