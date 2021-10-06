#!/bin/bash

WOOF=$1
TARGET=$2
IMG=$3

if ( test -z "$WOOF" ) ; then
	echo "usage curl-put woof server-ip path-to-image"
	exit 1
fi

if ( test -z "$TARGET" ) ; then
	echo "usage curl-put woof server-ip path-to-image"
	exit 1
fi

if ( test -z "$IMG" ) ; then
	echo "usage curl-put woof server-ip path-to-image"
	exit 1
fi

BIN=/home/smartfarm/bin

BW=`/usr/bin/curl -X POST -F "fileToUpload=@$IMG" $TARGET/upload.php | grep "elapsed" | awk '{print $7}'`

echo $BW | $BIN/senspot-put -W $WOOF -T d

