FROM centos:7
MAINTAINER Name rich@cs.ucsb.edu

RUN yum -y update && \
yum -y install epel-release

RUN yum -y install centos-release-scl && \
    yum -y install devtoolset-9

SHELL [ "/usr/bin/scl", "enable", "devtoolset-9" ]

RUN yum -y install zeromq-devel install \
	git \
	libtool \
	uuid-devel wget

WORKDIR /root

RUN wget https://github.com/ninja-build/ninja/releases/download/v1.10.1/ninja-linux.zip
RUN unzip ninja-linux.zip
RUN install ninja /usr/bin/

RUN wget https://github.com/Kitware/CMake/releases/download/v3.18.4/cmake-3.18.4-Linux-x86_64.tar.gz
RUN tar xf cmake-3.18.4-Linux-x86_64.tar.gz
RUN yes | cp -rf cmake-3.18.4-Linux-x86_64/* /usr/local/

RUN mkdir -p src
WORKDIR /root/src
RUN git clone https://github.com/zeromq/czmq.git
WORKDIR /root/src/czmq
RUN git checkout v4.2.0
RUN mkdir build
WORKDIR /root/src/czmq/build
RUN cmake -G Ninja \
    -DCZMQ_WITH_UUID=OFF \
    -DCZMQ_WITH_SYSTEMD=OFF \
    -DCZMQ_WITH_LZ4=OFF \
    -DCZMQ_WITH_LIBCURL=OFF \
    -DCZMQ_WITH_NSS=OFF \
    -DCZMQ_WITH_LIBMICROHTTPD=OFF \
    .. && \
    ninja && \
    ninja install
