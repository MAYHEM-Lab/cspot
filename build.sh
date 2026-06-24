#!/bin/bash
set -eu


# A easy script to build cspot. Requires docker. 
# Before running this, build the `cspot-compiler` docker image:
#
# sudo docker build -t cspot-compiler .
#
# Then, after building the image, simply run this script. 
# It will mount the repo in the container and output completed binaries in bin/
# of the repo. Be sure to run this in the root of the repo.
#
# The INSIDE_DOCKER_BUILD flag is for internal use. This is what the docker
# container calls

MODE="${1:-CALL_DOCKER}"

if [ "$MODE" = "INSIDE_DOCKER_BUILD" ]; then
    mkdir -p src
    mkdir -p build
    cd build
    cmake -G Ninja ..
    ninja 
    ninja install
else
    sudo docker run --rm -v "$PWD:/code" cspot-compiler
    sudo chown -R $USER:$USER build/bin
    sudo chown -R $USER:$USER build/bin/*
    echo "Binaries in $PWD/build/bin"
fi
