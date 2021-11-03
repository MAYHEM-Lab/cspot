#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

if ( test -z "$1" ) ; then
	echo "uptime-sensor.sh must specify target woof name"
	exit 1
fi

WNAME=$1

HERE=`pwd`

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
		echo $AVG | $HERE/senspot-put -W $WNAME -H senspot_log -T d
		$HERE/senspot-get -W $WNAME
	else
		echo $AVG | $HERE/senspot-put -W $WNAME -T d
	fi
else
	echo $AVG | $HERE/senspot-put -W $WNAME -T d
fi
		
		

