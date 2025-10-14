#!/bin/bash

# run this in the same directory where woofc-namespace-platform is running
HERE=`pwd`

ADDR=$1
COUNT=$2

if ( test -z "$ADDR" ) ; then
	echo "throughput-net.sh addr count"
	exit 1
fi

if ( test -z "$COUNT" ) ; then
        echo "throughput-net.sh count"
        exit 1
fi

./stress-init -W zzzstress -s $(($COUNT*2))

# do these in batches of 4 to stay under throttle limit

./stress-test -V -W woof://$ADDR/$PWD/zzzstress -s $COUNT -g 1 -p 1

