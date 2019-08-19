#!/bin/bash

if [[ $EUID -ne 0 ]]; then
   echo "run as root"
   exit 1
fi


GITHUBUSER=$1

if ( test -z "$GITHUBUSER" ) ; then
        echo "please specify github user to use"
        echo "usage: install.sh githubuserID"
        exit 1
fi

DTEST=`lsb_release -a | grep Distributor | awk '{print $3}'`

if ( test "$DEST" = "Ubuntu" ) ; then
	./install-client-only-ubuntu.sh
elif ( test "$DTEST" = "CentOS" ) ; then
	./install-client-only-centos7.sh
elif ( test "$DTEST" = "Raspbian" ) ; then
	./install-rasp.sh
else
	echo "Unknown distribution: " $DTEST
fi

