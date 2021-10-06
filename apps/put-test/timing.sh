#!/bin/bash

# run this in the same directory where woofc-namespace-platform is running
HERE=`pwd`

COUNT=$1

if ( test -z "$COUNT" ) ; then
	echo "timing.sh count"
	exit 1
fi

./stress-init -W woof://$HERE/zzzstress -s $(($COUNT*2))

# do these in batches of 4 to stay under throttle limit

BATCHCOUNT=$(($COUNT / 4))
CNT=0

while ( test $CNT -lt $BATCHCOUNT ) ; do
	./stress-test -W woof://$HERE/zzzstress -s 4 -g 1 -p 1	
	CNT=$(($CNT+1))
done

