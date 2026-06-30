#!/bin/bash

## install and build binary environment for CSPOT applications
# run this script in a directory under which you wish to create the build enviroment
# for a CSPO application

# get the binary update script for CSPOT
curl -fsSL https://raw.githubusercontent.com/MAYHEM-Lab/cspot/caplets/dist/update-cspot-distribution.sh > update-cspot-distribution.sh
chmod 755 update-cspot-distribution.sh

# install the CSPOT libraries and header files
# change "daily" to "release" for the release version
./update-cspot-distribution.sh daily lib

# check to see if MUSL is installed.  If it is, use it instead of gnu

if [[ -e "/opt/musl-cross/bin/x86_64-linux-musl-gcc" && -e "/opt/musl-cross/bin/x86_64-linux-musl-g++" ]] ; then
	cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=$PWD/toolchain-musl.cmake
else
	cmake -S . -B build
endif

build the application in the build directory
cd build
make
cd bin

#install the CSPOT binary runtime
cp ../../update-cspot-distribution.sh .
./update-cspot-distribution.sh daily

# start the namespace platform
./woofc-namespace-platform >& namespace.log &

# run the init side
./cspot-app-example-init -W test -s 1000

# run the client
./cspot-app-example-client -W test -S 100

# kill the platform
kill -HUP %1
