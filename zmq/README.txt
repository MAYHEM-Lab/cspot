launch the server

docker run -it -e LD_LIBRARY_PATH=/usr/local/lib -v /mnt/src/cspot/zmq:/zmq -p 6029:6029 cspot-docker-centos7 /zmq/zserver

then run zclient (must change local IP address)

