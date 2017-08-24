# cspot
c spot run

For a brief overview of CSPOT, please see the wiki at

https://github.com/MAYHEM-Lab/cspot/wiki/CSPOT-Run

CSPOT depends on two other libraries

https://github.com/richwolski/euca-cutils.git
https://github.com/MAYHEM-Lab/mio.git

The makefile for CSPOT assumes they are at the same level as the main cspot direcotry.  To build in some directory

git clone https://github.com/richwolski/euca-cutils.git
git clone https://github.com/MAYHEM-Lab/mio.git
git clone https://github.com/MAYHEM-Lab/cspot.git

cd euca-cutils; make
cd mio; make
cd cspot; make

CSPOT requires CZMQ.  If it isn't installed, there is a script (which must be run as root) in

cspot/zmq/install-zmq.sh

that will update the current installation
and install the necessary dependencies under CentOS 7.

Also, Docker will need to be installed and running.  Under CentOS 7,
running as root

yum -y install docker
systemctl start docker

All of the example applications codes ae in cspot/apps.  The build model assumes 
that the host machine is running CentOS 7 (see the github wiki for details)

The build process creates a Docker image in the local Docker repo for CSPOT with ZeroMQ and CZMQ installed.  
CZMQ is not supported as packages for CentOS 7 so the Dockerfile pulls it from github.  Also,
the default installation of CZMQ is in /usr/local/lib which is not set in the
the default LD_LIBRARY_PATH environment variable for CentOS 7.  Thus, to run any CSPOT binary
on the host, 

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

is necessary in the shell that launches the binary.

To enable rather verbose debugging in the CSPOT runtime, define DEBUG as a compile-time 
constant in the cspot source files and recompile.

The apps subdirectory contains several sample applications.  Their function is document on the CSPOT github wiki

https://github.com/MAYHEM-Lab/cspot/wiki/CSPOT-Run


