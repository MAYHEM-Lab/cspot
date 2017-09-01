#!/bin/bash

if ( test -z "$1" ) ; then
	echo "uptime-sensor.sh must specify target woof name"
	exit 1
fi

WNAME=$1

LINE=`uptime`
for WORD in $LINE ; do
	ATEST=`echo $WORD | grep '\.'`
	if ( ! test -z "$ATEST" ) ; then
		AVG=`echo $WORD | sed 's/,//g'`
		break
	fi
done

## second argument can be "-d" for debugging

if ( ! test -z "$2" ) ; then
	if ( test "$2" = "-d" ) ; then
		echo $AVG | senspot-put -W $WNAME -H senspot_log
		senspot-get -W $WNAME
	else
		echo $AVG | senspot-put -W $WNAME
	fi
else
	echo $AVG | senspot-put -W $WNAME
fi
		
		

