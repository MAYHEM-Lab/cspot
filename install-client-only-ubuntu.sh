#!/bin/bash

USER=$1

if [[ $EUID -ne 0 ]]; then
   echo "run as root"
   exit 1
fi

if [[ -z $USER ]]; then
  if [[ $# -ne 1 ]]; then
    echo "usage: ./install-client-only-ubuntu.sh <github-username>"
    exit 1
  else
    USER=$1
  fi
fi

HERE=`pwd`

apt-get install -y gfortran
cd ..
git clone https://github.com/richwolski/euca-cutils.git
git clone https://$USER@github.com/MAYHEM-Lab/mio.git

cd euca-cutils; make
cd ..
cd mio; make
cd $HERE

cd zmq
./install-ubuntu-zmq.sh
cd ..
make abd
