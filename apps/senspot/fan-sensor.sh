#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

HERE=/root/bin

if ( test -z "$1" ) ; then
	echo "core-temp-sensor.sh must specify target woof name"
	exit 1
fi

DISTRO="CENTOS"

WNAME=$1

if ( test "$DISTRO" == "debian" ) ; then
        TEMP1=`/opt/vc/bin/vcgencmd measure_temp | sed 's/temp=//' | awk -F "'"  '{print ($1*9/5)+32}'`
        TEMP=`cat /sys/class/thermal/thermal_zone0/temp | awk '{print (($1/1000)*9/5)+32}'`
else
# yum -y install lm_sensors
# sensors-detect
#        TEMP=`sensors -f | grep "Package" | awk '{print $4}' | sed 's/+//' | cat -v | sed 's/M-BM-0F//'`
        TEMP=`sensors -f | grep "fan2:" | awk '{print $2}'`
fi


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
		
		

