#!/bin/bash

apt-get update && apt-get -y install ninja-build g++-9 cmake g++
git submodule update --init --recursive
mkdir -p build
cd build/
cmake -DCMAKE_CXX_COMPILER=/usr/bin/g++-9 ..
make
make install
#if ! [[ $LD_LIBRARY_PATH == *"/usr/local/lib"* ]]; then
#    echo -e "if ! [[ \$LD_LIBRARY_PATH == *\"/usr/local/lib\"* ]]; then\nexport  LD_LIBRARY_PATH=\"\$LD_LIBRARY_PATH:/usr/local/lib\"\nfi" >> ~/.bashrc
#    source ~/.bashrc
#fi
#mkdir -p /home/ubuntu/bin
#cp -r bin/* /home/ubuntu/bin
#chown -R ubuntu:ubuntu /home/ubuntu/bin
#echo -e "if ! [[ \$LD_LIBRARY_PATH == *\"/usr/local/lib\"* ]]; then\nexport  LD_LIBRARY_PATH=\"\$LD_LIBRARY_PATH:/usr/local/lib\"\nfi" >> /home/ubuntu/.bashrc
#echo -e "export  PATH=\"\$PATH:/usr/local/bin:/home/ubuntu/bin\"" >> /home/ubuntu/.bashrc
#echo "Please run this as the ubuntu to update your environment variables: source ~/.bashrc"
