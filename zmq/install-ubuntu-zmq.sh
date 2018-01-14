#!/bin/bash

wget https://github.com/zeromq/libzmq/releases/download/v4.2.1/zeromq-4.2.1.tar.gz
tar -xvzf zeromq-4.2.1.tar.gz
cd zeromq-4.2.1/

apt-get -y install libtool pkg-config build-essential autoconf automake uuid-dev
apt-get -y install checkinstall

./configure
make
make install
checkinstall
ldconfig

cd ..

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
mkdir -p /src
cd /src
git clone git://github.com/zeromq/czmq.git
cd czmq
./autogen.sh && ./configure && make
make install
ldconfig
