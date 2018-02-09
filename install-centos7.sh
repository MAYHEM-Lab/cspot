#!/bin/bash


HERE=`pwd`
yum -y install docker gcc-gfortran
cd ..
git clone https://github.com/richwolski/euca-cutils.git
git clone https://richwolski@github.com/MAYHEM-Lab/mio.git
git clone https://richwolski@github.com/MAYHEM-Lab/matrix.git
git clone https://richwolski@github.com/MAYHEM-Lab/distributions.git
git clone https://richwolski@github.com/MAYHEM-Lab/ssa-changepoint.git

cd euca-cutils; make
cd ..
cd mio; make
cd ..
cd distributions; make
cd ..
cd matrix; ./install-matrix.sh
cd ..
cd ssa-changepoint; make
cd $HERE

systemctl start docker
systemctl enable docker

cd zmq
./install-zmq.sh
cd ..
make
