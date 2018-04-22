#!/bin/bash

USER=richwolski

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
apt install docker gfortran
cd ..
git clone https://github.com/richwolski/euca-cutils.git
git clone https://$USER@github.com/MAYHEM-Lab/mio.git
git clone https://$USER@github.com/MAYHEM-Lab/distributions.git
git clone https://$USER@github.com/MAYHEM-Lab/matrix.git
git clone https://$USER@github.com/MAYHEM-Lab/ssa-changepoint.git

cd euca-cutils; make
cd ..
cd mio; make
cd ..
cd distributions; make
cd ..
cd matrix; ./install-ubuntu-matrix.sh
cd ..
cd ssa-changepoint; make
cd $HERE

systemctl start docker
systemctl enable docker

cd zmq
./install-ubuntu-zmq.sh
cd ..
make
