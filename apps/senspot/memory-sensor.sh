#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

if ( test -z "$1" ) ; then
	echo "uptime-sensor.sh must specify target woof name"
	exit 1
fi

WNAME=$1

HERE=/root/bin

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

TOT=`vmstat -s | grep "total memory" | awk '{print $1}'`
USED=`vmstat -s | grep "used memory" | awk '{print $1}'`

FRAC=`echo $TOT $USED | awk '{print $2/$1}'`

echo $FRAC |  /root/bin/senspot-put -W $1 -T d

		
		

