#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

HERE=/root/bin

if ( test -z "$1" ) ; then
	echo "core-temp-sensor.sh must specify target woof name"
	exit 1
fi

WNAME=$1

TEMP=`sensors -f | grep "Package" | awk '{print $4}' | sed 's/+//' | cat -v | sed 's/M-BM-0F//'`


## second argument can be "-d" for debugging

if ( ! test -z "$2" ) ; then
	if ( test "$2" = "-d" ) ; then
		echo $TEMP | $HERE/senspot-put -W $WNAME -H senspot_log -T d
		$HERE/senspot-get -W $WNAME
	else
		echo $TEMP | $HERE/senspot-put -W $WNAME -T d
	fi
else
	echo $TEMP | $HERE/senspot-put -W $WNAME -T d
fi
		
		

