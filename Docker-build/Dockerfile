FROM centos:7
MAINTAINER Name rich@cs.ucsb.edu
RUN yum -y update && \
yum -y install epel-release && \
yum -y install zeromq-devel install blas blas-devel \
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
ENV LAPACK=lapack-3.8.0
RUN mkdir ${LAPACK}
RUN /bin/bash -l -c '\
    curl -sSL "http://www.netlib.org/lapack/${LAPACK}.tar" | \
    tar xz && cd ${LAPACK} && \
    cp make.inc.example make.inc && \
    make blaslib && \
    make lapacklib -j 8 && cp liblapack.a /lib64 && \
    cp liblapack.a /usr/local/lib && cp librefblas.a /usr/local/lib && \
    cd LAPACKE && make && cd .. && cp liblapacke.a /usr/local/lib && \
    cd ../ && rm -rf ${LAPACK}'
