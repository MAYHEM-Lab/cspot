#!/bin/bash

DTEST=`lsb_release -a | grep Distributor | awk '{print $3}'`

if ( test "$DEST" = "Ubuntu" ) ; then
	./install-ubuntu.sh
elif ( test "$DTEST" = "CentOS" ) ; then
	./install-centos7.sh
elif ( test "$DTEST" = "Raspbian" ) ; then
	./install-rasp.sh
else
	echo "Unknown distribution: " $DTEST
fi

