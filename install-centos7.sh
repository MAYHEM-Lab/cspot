#!/bin/bash

HERE=`pwd`
yum -y install gcc-gfortran python-devel python34-devel
yum -y remove docker docker-client docker-client-latest docker-common docker-latest docker-latest-logrotate docker-logrotate docker-engine
yum install -y yum-utils
yum-config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo
yum -y install docker-ce docker-ce-cli containerd.io
cd ..
git clone https://github.com/richwolski/euca-cutils.git
git clone https://github.com/MAYHEM-Lab/mio.git
git clone https://github.com/MAYHEM-Lab/matrix.git
git clone https://github.com/MAYHEM-Lab/distributions.git
git clone https://github.com/MAYHEM-Lab/ssa-changepoint.git

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
