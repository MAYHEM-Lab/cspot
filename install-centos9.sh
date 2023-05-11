#!/bin/bash


dnf -y install gcc g++ unzip ninja-build cmake
dnf -y install glibc-static unzip
dnf -y --enablerepo=crb install libstdc++-static
dnf -y remove docker docker-client docker-client-latest docker-common docker-latest docker-latest-logrotate docker-logrotate docker-engine
dnf -y install yum-utils
dnf -y install wget
yum -y erase podman buildah
yum-config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo
yum -y install docker-ce docker-ce-cli containerd.io
systemctl start docker
cd ..
#wget https://github.com/ninja-build/ninja/releases/download/v1.10.2/ninja-linux.zip
#unzip ninja-linux.zip 
#mv ninja /usr/bin/
#wget https://github.com/Kitware/CMake/releases/download/v3.19.1/cmake-3.19.1-Linux-x86_64.sh
#chmod +x cmake-3.19.1-Linux-x86_64.sh 
#./cmake-3.19.1-Linux-x86_64.sh --skip-license --prefix=/usr
yum -y localinstall https://download-ib01.fedoraproject.org/pub/epel/7/x86_64/Packages/c/czmq-3.0.2-3.el7.x86_64.rpm
cd cspot/
git submodule update --init --recursive
mkdir build
cd build/
source ~/.bashrc
cmake -G Ninja ..
ninja
ninja install
#scl enable devtoolset-9 ./helper.sh
docker pull racelab/cspot-docker-centos7
docker tag racelab/cspot-docker-centos7 cspot-docker-centos7

if ! [[ $LD_LIBRARY_PATH == *"/usr/local/lib"* ]]; then
    echo -e "if ! [[ \$LD_LIBRARY_PATH == *\"/usr/local/lib\"* ]]; then\nexport  LD_LIBRARY_PATH=\"\$LD_LIBRARY_PATH:/usr/local/lib\"\nfi" >> ~/.bashrc
    source ~/.bashrc
fi
cp ../SELF-TEST.sh ./bin
cd ./bin
./SELF-TEST.sh
