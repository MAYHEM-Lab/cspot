#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

BIN=/root/bin

if ( test -z "$1" ) ; then
	echo "read-archive.sh woof name file name"
	exit 1
fi

if ( test -z "$2" ) ; then
	echo "read-archive.sh woof name file name"
	exit 1
fi
	
WNAME=$1
FNAME=$2

LINE=`$BIN/senspot-get -W $WNAME`

if ( test -z "$LINE" ) ; then
	echo "couldn't read last value from $WNAME"
	exit 1
fi

LASTSEQNO=`echo $LINE | awk '{print $6}'`

if ( ! test -e "$FNAME" ) ; then
	READINGS=$LASTSEQNO
	START=1
else
	LASTFSEQNO=`tail -n 1 $FNAME | awk '{print $6}'`
	READINGS=$(($LASTSEQNO - $LASTFSEQNO))
	START=$(($LASTFSEQNO+1))
fi

if ( test $READINGS -le 0) ; then
	echo $WNAME up to date with $FNAME
	exit 0
fi

CNT=0

UN=`uname`
if ( test "$UN" = "Linux" ) ; then
	ISLINUX=T
else
	ISLINUX=
fi

SEQNO=$START
while ( test $CNT -lt $READINGS ) ; do
	LINE=`$BIN/senspot-get -W $WNAME -S $SEQNO`
	if ( test -z "$LINE" ) ; then
		CNT=$(($CNT+1))
		SEQNO=$(($SEQNO+1))
		continue
	fi
	TIME=`echo $LINE | awk '{print $3}' | sed 's/\./ /' | awk '{print $1}'`
	if ( test -z "$ISLINUX" ) ; then
		DT=`date -r $TIME`
	else
		DT=`date --date @$TIME`
	fi
	echo $LINE "--" $DT >> $FNAME
	CNT=$(($CNT+1))
	SEQNO=$(($SEQNO+1))
done
		
		
		

