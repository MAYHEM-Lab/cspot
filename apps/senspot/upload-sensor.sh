#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

HERE=/root/bin
BIN=/usr/bin

if ( test -z "$1" ) ; then
	echo "upload-sensor.sh must specify target woof name"
	echo "upload-sensor woof_name IP-addre-of-iperf3-server"
	exit 1
fi

WNAME=$1

if ( test -z "$2" ) ; then
	echo "upload-sensor.sh must specify target server"
	echo "upload-sensor woof_name IP-addre-of-iperf3-server"
	exit 1
fi

ADDR=$2

OUTLINE=`$BIN/iperf3 -c $ADDR -p 8008 | grep receiver`

CNT=0
MAX=6
while ( test -z "$OUTLINE" ) ; do
	sleep 10
	OUTLINE=`$BIN/iperf3 -c $ADDR -p 8008 | grep receiver`
	CNT=$(($CNT+1))
	if ( test $CNT -ge $MAX ) ; then
		break;
	fi
done


AVG=`echo $OUTLINE | awk '{print $7}'`
UNITS=`echo $OUTLINE | awk '{print $8}'`

# units have to be megabits

if ( test "$UNITS" = "Kbits/sec" ) ; then
        NAVG=`echo $AVG | awk '{print $1 / 1000.0}'`
        AVG=$NAVG
fi


## third argument can be "-d" for debugging

if ( ! test -z "$3" ) ; then
	if ( test "$3" = "-d" ) ; then
		echo $AVG | $HERE/senspot-put -W $WNAME -H senspot_log -T d
		$HERE/senspot-get -W $WNAME
	else
		echo $AVG | $HERE/senspot-put -W $WNAME -T d
	fi
else
	echo $AVG | $HERE/senspot-put -W $WNAME -T d
fi
		
		

