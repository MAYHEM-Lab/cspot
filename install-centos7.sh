#!/bin/bash

if [[ $EUID -ne 0 ]]; then
   echo "run as root"
   exit 1
fi


GITHUBUSER=$1

if ( test -z "$GITHUBUSER" ) ; then
        echo "please specify github user to use"
        echo "usage: install-centos7.sh githubuserID"
        exit 1
fi


HERE=`pwd`
yum -y install docker gcc-gfortran python-devel python34-devel
cd ..
git clone https://github.com/richwolski/euca-cutils.git
git clone https://$GITHUBUSER@github.com/MAYHEM-Lab/mio.git
git clone https://$GITHUBUSER@github.com/MAYHEM-Lab/matrix.git
git clone https://$GITHUBUSER@github.com/MAYHEM-Lab/distributions.git
git clone https://$GITHUBUSER@github.com/MAYHEM-Lab/ssa-changepoint.git

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
