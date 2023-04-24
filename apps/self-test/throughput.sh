#!/bin/bash

# run this in the same directory where woofc-namespace-platform is running
HERE=`pwd`

COUNT=$1

if ( test -z "$COUNT" ) ; then
        echo "throughput.sh count"
        exit 1
fi

./stress-init -W zzzstress -s $(($COUNT*2))

# do these in batches of 4 to stay under throttle limit

./stress-test -V -L -W zzzstress -s $COUNT -g 1 -p 1

