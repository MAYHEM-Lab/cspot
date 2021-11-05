FROM centos:7
MAINTAINER Name rich@cs.ucsb.edu
#RUN sed 's/mirrorlist=/#mirrorlist=/g' /etc/yum.repos.d/CentOS-Base.repo | sed 's/#baseurl/baseurl/g' > /tmp/CentOS-Base.repo
#RUN cp /tmp/CentOS-Base.repo /etc/yum.repos.d/CentOS-Base.repo
RUN yum -y update && yum -y install epel-release
RUN yum -y install zeromq-devel install blas blas-devel atlas-devel \
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
RUN echo "#define ZMQ_ROUTING_ID (0)" > /root/src/czmq/tmp.h
RUN cat /root/src/czmq/tmp.h /root/src/czmq/include/czmq.h > /root/src/czmq/tmp1.h
RUN cp /root/src/czmq/tmp1.h /root/src/czmq/include/czmq.h
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
# install python36 for AWS Lambda execution
#RUN yum install -y https://repo.ius.io/ius-release-el7.rpm && \
#    yum update -y && \
#    yum install -y python36u python36u-libs python36u-devel python36u-pip \
