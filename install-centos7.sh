#!/bin/bash

USEMAKE=0

yum -y install centos-release-scl && yum -y install devtoolset-9
yum -y install centos-release-scl && sudo yum -y install devtoolset-9
yum -y install glibc-static openssl-static openssl-devel zlib-static
#yum -y remove docker docker-client docker-client-latest docker-common docker-latest docker-latest-logrotate docker-logrotate docker-engine
#yum install -y yum-utils
#yum-config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo
#yum -y install docker-ce docker-ce-cli containerd.io
#systemctl start docker
cd ..
wget https://github.com/ninja-build/ninja/releases/download/v1.10.2/ninja-linux.zip
unzip ninja-linux.zip 
mv ninja /usr/bin/
wget https://github.com/Kitware/CMake/releases/download/v3.19.1/cmake-3.19.1-Linux-x86_64.sh
chmod +x cmake-3.19.1-Linux-x86_64.sh 
./cmake-3.19.1-Linux-x86_64.sh --skip-license --prefix=/usr
yum -y localinstall https://download-ib01.fedoraproject.org/pub/epel/7/x86_64/Packages/c/czmq-3.0.2-3.el7.x86_64.rpm

#git clone --branch release-2.0 https://github.com/Mayhem-lab/cspot
cd cspot/
git submodule update --init --recursive
#mkdir build
#cd build/
#
#echo "#!/bin/bash" > helper.sh
#if ( test USEMAKE -eq 0 ) ; then
#	echo "cmake -G Ninja .." >> helper.sh
#	echo "ninja" >> helper.sh
#	echo "ninja install" >> helper.sh
#else
#	mkdir -p bin
#	echo "cmake .." >> helper.sh
#	echo "make" >> helper.sh
#	echo "make install" >> helper.sh
#fi
#chmod 755 helper.sh
#scl enable devtoolset-9 ./helper.sh
#docker pull racelab/cspot-docker-centos7
#docker tag racelab/cspot-docker-centos7 cspot-docker-centos7

if ! [[ $LD_LIBRARY_PATH == *"/usr/local/lib"* ]]; then
    echo -e "if ! [[ \$LD_LIBRARY_PATH == *\"/usr/local/lib\"* ]]; then\nexport  LD_LIBRARY_PATH=\"\$LD_LIBRARY_PATH:/usr/local/lib\"\nfi" >> ~/.bashrc
    source ~/.bashrc
fi
#cp ../SELF-TEST.sh ./bin
#cd bin
#./SELF-TEST.sh

