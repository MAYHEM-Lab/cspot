#!/bin/bash

ME=`whoami`
ISADMIN=`id -G $ME | grep ' 80 '`
if ( test -z "$ISADMIN" ) ; then
	echo "must be admin user under OSX to install deps
	exit 1
fi 


HERE=`pwd`
cd ..
git clone https://github.com/richwolski/euca-cutils.git
git clone https://github.com/MAYHEM-Lab/mio.git

cd euca-cutils; make
cd ..
cd mio; make
cd $HERE

brew install zeromq
brew install czmq
make abd
