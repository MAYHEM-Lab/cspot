#!/bin/bash
# must be run as root

# uassumes musl is set in basrc and openssl has been built with it

apt-get update
apt install -y build-essential git gcc make wget cmake
apt install gawk bison flex texinfo
cd ..
git clone https://github.com/richfelker/musl-cross-make.git
cd musl-cross-make/
cat > config.mak <<EOF
TARGET = x86_64-linux-musl
OUTPUT = /opt/musl-cross
EOF
make
make install
cd ..
# all subsequent compiles and links use musl
export PATH=/opt/musl-cross/bin:$PATH
export CC=x86_64-linux-musl-gcc
export AR=x86_64-linux-musl-ar
export RANLIB=x86_64-linux-musl-ranlib
export CFLAGS="-static"
wget https://www.openssl.org/source/openssl-3.1.4.tar.gz
tar -xzf openssl-3.1.4.tar.gz
cd openssl-3.1.4
./Configure linux-x86_64 no-shared no-dso --prefix=/opt/openssl-musl
make
make install
cd ..
wget https://pyyaml.org/download/libyaml/yaml-0.2.5.tar.gz
tar -xzf yaml-0.2.5.tar.gz
make
make install
cd ../cspot
git submodule update --init --recursive
mkdir -p build
cd build/
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-musl.cmake ..
make
