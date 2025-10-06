#!/bin/bash
#
source ~/.bashrc

# uassumes musl is set in basrc and openssl has been built with it

apt-get update
apt install -y build-essential git gcc make wget
apt install gawk bison flex texinfo
git clone https://github.com/richfelker/musl-cross-make.git
cd musl-cross-make/
cat > config.mak <<EOF
TARGET = x86_64-linux-musl
OUTPUT = /opt/musl-cross
EOF
make
make install
cd ..
wget https://www.openssl.org/source/openssl-3.1.4.tar.gz
tar -xzf openssl-3.1.4.tar.gz
cd openssl-3.1.4
./Configure linux-x86_64 no-shared no-dso --prefix=/opt/openssl-musl
wget https://pyyaml.org/download/libyaml/yaml-0.2.5.tar.gz
git submodule update --init --recursive
mkdir -p build
cd build/
cmake ..
#cmake -DCMAKE_CXX_COMPILER=/usr/bin/g++-9 ..
#make
#make install
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
