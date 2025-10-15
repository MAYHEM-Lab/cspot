#!/bin/bash

# run this in the same directory where woofc-namespace-platform is running

COUNT=$2
BSIZE=1
ADDR=$1
if ( test -z "$3" ) ; then
	HERE=`pwd`
else
	HERE=/home/ubuntu/cspot/build/bin
fi

if ( test -z "$ADDR" ) ; then
	echo "latency-net.sh IP-addr count"
	exit 1
fi

if ( test -z "$COUNT" ) ; then
	echo "latency-net.sh IP-addr count"
	exit 1
fi

./stress-init -W zzzstress -s $(($COUNT*2))

# do these in batches of 4 to stay under throttle limit

BATCHCOUNT=$(($COUNT / $BSIZE))
CNT=0

while ( test $CNT -lt $BATCHCOUNT ) ; do
        ./stress-test -l -W woof://$ADDR/$HERE/zzzstress -s $BSIZE -g 1 -p 1
        CNT=$(($CNT+1))
done

