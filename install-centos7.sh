#!/bin/bash


HERE=`pwd`
cd ..
git clone https://github.com/richwolski/euca-cutils.git
git clone https://richwolski@github.com/MAYHEM-Lab/mio.git

cd euca-cutils; make
cd ..
cd mio; make
cd $HERE

yum -y install docker
systemctl start docker
systemctl enable docker

cd zmq
./install-zmq.sh
cd ..
make
