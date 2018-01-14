#!/bin/bash

wget https://github.com/zeromq/libzmq/releases/download/v4.2.1/zeromq-4.2.1.tar.gz
tar -xvzf zeromq-4.2.1.tar.gz
cd zeromq-4.2.1/

sudo apt-get install libtool pkg-config build-essential autoconf automake uuid-dev
sudo apt-get install checkinstall

./configure
make
sudo checkinstall
sudo ldconfig

cd ..

mkdir -p /src
cd /src
git clone git://github.com/zeromq/czmq.git
cd czmq
./autogen.sh && ./configure && make check
make install
ldconfig
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
