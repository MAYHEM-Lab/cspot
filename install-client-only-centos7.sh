#!/bin/bash

if [[ $EUID -ne 0 ]]; then
   echo "run as root"
   exit 1
fi


GITHUBUSER=$1

if ( test -z "$GITHUBUSER" ) ; then
        echo "please specify github users to use"
        echo "usage: install-client-only-centos7.sh githubuserID"
        exit 1
fi

HERE=`pwd`
yum -y install gcc-gfortran 
cd ..
git clone https://github.com/richwolski/euca-cutils.git
git clone https://$GITHUBUSER@github.com/MAYHEM-Lab/mio.git

cd euca-cutils; make
cd ..
cd mio; make
cd $HERE

cd zmq
./install-zmq.sh
cd ..
make abd
