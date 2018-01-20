FROM centos:7
MAINTAINER Name rich@cs.ucsb.edu
RUN yum -y update && \
yum -y install epel-release && \
yum -y install zeromq-devel \
	gcc \
	autoconf \
	gdb \
	git \
	make \
	cmake \ 
	libtool \
	uuid-devel 
WORKDIR /root
RUN mkdir -p src
WORKDIR /root/src
RUN git clone git://github.com/zeromq/czmq.git
WORKDIR /root/src/czmq
RUN ./autogen.sh && ./configure && make && make install && ldconfig
WORKDIR /
