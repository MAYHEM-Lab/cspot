#!/bin/bash

# run this in the same directory where woofc-namespace-platform is running
HERE=`pwd`

COUNT=$1

if ( test -z "$COUNT" ) ; then
	echo "timing.sh count"
	exit 1
fi

if ( test -z "$2" ) ; then
	PAYLOAD=1500
else
	PAYLOAD=$2
fi

./stress-init -W woof://$HERE/zzzstress -s $(($COUNT*2)) -P $PAYLOAD

# do these in batches of 4 to stay under throttle limit

BATCHCOUNT=$(($COUNT / 4))
CNT=0

TOTAL=0
while ( test $CNT -lt $BATCHCOUNT ) ; do
	SUM4=`./stress-test -W woof://$HERE/zzzstress -s 4 -g 1 -p 1 -P $PAYLOAD | grep elapsed | awk '{print $4}'`
	SUM=`echo $SUM4 | awk '{print $1+$2+$3+$4}'`
#echo SUM4: $SUM4 SUM: $SUM
	TOTAL=`echo $TOTAL $SUM  | awk '{print $1+$2}'`
	CNT=$(($CNT+1))
done
BW=`echo $TOTAL $PAYLOAD $COUNT | awk '{print ($2*$3)/($1/1000.0)}'`
echo payload: $PAYLOAD count: $COUNT time: $TOTAL bw: $BW bytes/s

