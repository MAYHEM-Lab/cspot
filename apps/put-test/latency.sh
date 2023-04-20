#!/bin/bash

# run this in the same directory where woofc-namespace-platform is running
HERE=`pwd`

COUNT=$1
BSIZE=1

if ( test -z "$COUNT" ) ; then
        echo "timing.sh count"
        exit 1
fi

./stress-init -W zzzstress -s $(($COUNT*2))

# do these in batches of 4 to stay under throttle limit

BATCHCOUNT=$(($COUNT / $BSIZE))
CNT=0

while ( test $CNT -lt $BATCHCOUNT ) ; do
        ./stress-test -l -L -W zzzstress -s $BSIZE -g 1 -p 1
        CNT=$(($CNT+1))
done

