#!/bin/bash

if [[ $EUID -ne 0 ]]; then
   echo "run as root"
   exit 1
fi


# must be run as root to install the dependencies needed for CentOS 7
# will install UCSB deps from github
#
# useful for a build server

GITHUBUSER=$1

if ( test -z "$GITHUBUSER" ) ; then
	echo "please specify github users to use"
	echo "usage: install-deps-centos7.sh githubuserID"
	exit 1
fi


yum -y update
yum -y install docker gcc-gfortran python-devel python34-devel yum-utils device-mapper-persistent-data lvm2
yum-config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo
yum -y install docker-ce

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
