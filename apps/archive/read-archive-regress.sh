#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

BIN=/root/bin

if ( test -z "$1" ) ; then
	echo "read-archive-regress.sh woof name file name"
	exit 1
fi

if ( test -z "$2" ) ; then
	echo "read-archive-regress.sh woof name file name"
	exit 1
fi
	
WNAME=$1
FNAME=$2

LINE=`$BIN/regress-pair-get -W $WNAME -s 'a'`

if ( test -z "$LINE" ) ; then
	echo "couldn't read last value from $WNAME"
	exit 1
fi

LASTSEQNO=`echo $LINE | awk '{print $4}'`

if ( test -z "LINE" ) ; then
	echo "no last seq no for $WNAME"
	exit 1
fi

TIME=`echo $LINE | awk '{print $3}' | sed 's/\./ /' | awk '{print $1}'`
DT=`date --date @$TIME`

if ( ! test -e "$FNAME" ) ; then
	echo $LINE -- $DT > $FNAME
	exit 0
else
	LASTFSEQNO=`tail -n 1 $FNAME | awk '{print $4}'`
	if ( test $LASTSEQNO -eq $LASTFSEQNO ) ; then
		echo $WNAME up to date with $FNAME
		exit 0
	fi
fi

echo $LINE -- $DT >> $FNAME

