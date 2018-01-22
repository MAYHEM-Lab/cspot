#!/bin/bash


HERE=`pwd`
cd ..
git clone https://github.com/richwolski/euca-cutils.git
git clone https://richwolski@github.com/MAYHEM-Lab/mio.git
git clone https://richwolski@github.com/MAYHEM-Lab/matrix.git

cd euca-cutils; make
cd ..
cd mio; make
cd $HERE

yum -y install docker gcc-gfortran
systemctl start docker
systemctl enable docker

cd zmq
./install-zmq.sh
cd ..
make
