#!/bin/bash


KEY=$1
STATION=$2
FILE=$3

if ( test -z "$KEY" ) ; then
	echo "wu-get key station file"
	exit 1
fi

if ( test -z "$STATION" ) ; then
	echo "wu-get key station file"
	exit 1
fi

if ( test -z "$FILE" ) ; then
	echo "wu-get key station file"
	exit 1
fi

JSON=`wget -O - http://api.wunderground.com/api/$KEY/conditions/q/pws:$STATION.json`
TS=`echo $JSON | sed 's/\[//' | sed 's/,//g' | awk '{$1=$1}1' OFS="\n" | sed 's/\]//g' | grep local_epoch | sed 's/"//g' | awk -F ':' '{print $2}'`
TEMP=`echo $JSON | sed 's/\[//' | sed 's/,//g' | awk '{$1=$1}1' OFS="\n" | sed 's/\]//g' | grep temp_f| sed 's/"//g' | awk -F ':' '{print $2}'`

if ( ! test -e "$FILE" ) ; then
	echo $TS $TEMP > $FILE
	exit 0
fi
LAST_TS=`tail -n 1 $FILE | awk '{print $1}'`

if ( test $TS -le $LAST_TS ) ; then
	exit 0
fi

if ( ! test -z "$TS" ) ; then
	echo $TS $TEMP >> $FILE
fi

exit 0
