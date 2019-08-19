#!/bin/bash

# client only script for raspbian
# for now, no container support -- soon

USER=$1

if [[ $EUID -ne 0 ]]; then
   echo "run as root"
   exit 1
fi

if [[ -z $USER ]]; then
  if [[ $# -ne 1 ]]; then
    echo "usage: ./install-ubuntu.sh <github-username>"
    exit 1
  else
    USER=$1
  fi
fi


HERE=`pwd`
cd ..
git clone git@github.com:richwolski/euca-cutils.git
git clone git@github.com:MAYHEM-Lab/mio.git

cd euca-cutils; make
cd ..
cd mio; make
cd $HERE

#yum -y install docker
#systemctl start docker
#systemctl enable docker

cd zmq
./install-ubuntu-zmq.sh
cd ..
make abd
