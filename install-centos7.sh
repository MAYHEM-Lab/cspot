#!/bin/bash


HERE=`pwd`
yum -y install docker gcc-gfortran python-devel python34-devel
cd ..
git clone git@github.com:richwolski/euca-cutils.git
git clone git@github.com:MAYHEM-Lab/mio.git
git clone git@github.com:MAYHEM-Lab/matrix.git
git clone git@github.com:MAYHEM-Lab/distributions.git
git clone git@github.com:MAYHEM-Lab/ssa-changepoint.git

cd euca-cutils; make -j12
cd ..
cd mio; make -j12
cd ..
cd distributions; make -j12
cd ..
cd matrix; ./install-matrix.sh
cd ..
cd ssa-changepoint; make -j12
cd $HERE

systemctl start docker
systemctl enable docker

cd zmq
./install-zmq.sh
cd ..
make -j12
