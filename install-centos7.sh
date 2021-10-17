#!/bin/bash


yum -y install centos-release-scl && yum -y install devtoolset-9
yum -y install centos-release-scl && sudo yum -y install devtoolset-9
yum -y install glibc-static
yum -y remove docker docker-client docker-client-latest docker-common docker-latest docker-latest-logrotate docker-logrotate docker-engine
yum install -y yum-utils
yum -y install wegt
yum-config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo
yum -y install docker-ce docker-ce-cli containerd.io
cd ..
wget https://github.com/ninja-build/ninja/releases/download/v1.10.2/ninja-linux.zip
unzip ninja-linux.zip 
mv ninja /usr/bin/
wget https://github.com/Kitware/CMake/releases/download/v3.19.1/cmake-3.19.1-Linux-x86_64.sh
chmod +x cmake-3.19.1-Linux-x86_64.sh 
./cmake-3.19.1-Linux-x86_64.sh --skip-license --prefix=/usr
#git clone --branch release-2.0 https://github.com/Mayhem-lab/cspot
cd cspot/
git submodule update --init --recursive
mkdir build
cd build/
echo "#!/bin/bash" > helper.sh
echo "cmake -G Ninja .." >> helper.sh
echo "ninja" >> helper.sh
echo "ninja install" >> helper.sh
chmod 755 helper.sh
scl enable devtoolset-9 ./helper.sh
docker pull racelab/cspot-docker-centos7

