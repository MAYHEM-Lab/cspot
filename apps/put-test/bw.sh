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
MIN=9999999999
while ( test $CNT -lt $BATCHCOUNT ) ; do
	SUM4=`./stress-test -W woof://$HERE/zzzstress -s 4 -g 1 -p 1 -P $PAYLOAD | grep elapsed | awk '{print $4}'`
	MIN=`echo $SUM4 | awk -v min=$MIN '{if($1 < min){print $1}else{print min}}'`
	MIN=`echo $SUM4 | awk -v min=$MIN '{if($2 < min){print $2}else{print min}}'`
	MIN=`echo $SUM4 | awk -v min=$MIN '{if($3 < min){print $3}else{print min}}'`
	MIN=`echo $SUM4 | awk -v min=$MIN '{if($4 < min){print $4}else{print min}}'`
	SUM=`echo $SUM4 | awk '{print $1+$2+$3+$4}'`
#echo SUM4: $SUM4 SUM: $SUM
	TOTAL=`echo $TOTAL $SUM  | awk '{print $1+$2}'`
	CNT=$(($CNT+1))
done
BW=`echo $TOTAL $PAYLOAD $COUNT | awk '{print ($2*$3)/($1/1000.0)}'`
BEST=`echo $PAYLOAD $MIN | awk '{print $1 / ($2 / 1000.0)}'`
echo payload: $PAYLOAD count: $COUNT time: $TOTAL bw: $BW bytes/s min: $MIN best: $BEST bytes/s

