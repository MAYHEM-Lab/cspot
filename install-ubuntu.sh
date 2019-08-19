#!/bin/bash

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

apt-get update
apt-get install -y gfortran curl make cmake g++
apt-get remove -y docker docker-engine docker.io
apt-get install \
    apt-transport-https \
    ca-certificates \
    curl \
    software-properties-common

curl -fsSL https://download.docker.com/linux/ubuntu/gpg | apt-key add -
apt-key fingerprint 0EBFCD88
add-apt-repository -y \
   "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
   $(lsb_release -cs) \
   stable"
apt-get update
apt-get install -y docker-ce

cd ..
git clone git@github.com:richwolski/euca-cutils.git
git clone git@github.com:MAYHEM-Lab/mio.git
git clone git@github.com:MAYHEM-Lab/matrix.git
git clone git@github.com:MAYHEM-Lab/distributions.git
git clone git@github.com:MAYHEM-Lab/ssa-changepoint.git

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
