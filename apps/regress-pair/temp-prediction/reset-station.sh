#!/bin/bash

WOOF=$1
SIZE=$2
COUNTBACK=$3
STATIONFILE=$4
EXPWOOF=$5

BIN=/smartedge/bin
WOOFDIR=`pwd`

if ( test -z "$WOOF" ) ; then
	echo  "reset-station.sh woof-name woof-size countback station-file explain-woof"
	exit 1
fi

if ( test -z "$SIZE" ) ; then
	echo  "reset-station.sh woof-name woof-size countback station-file explain-woof"
	exit 1
fi

if ( test -z "$COUNTBACK" ) ; then
	echo  "reset-station.sh woof-name woof-size countback station-file explain-woof"
	exit 1
fi

if ( test -z "$STATIONFILE" ) ; then
	echo  "reset-station.sh woof-name woof-size countback station-file explain-woof"
	exit 1
fi

if ( test -z "$EXPWOOF" ) ; then
	echo  "reset-station.sh woof-name woof-size countback station-file explain-woof"
	exit 1
fi

HERE=`pwd`
cd $WOOFDIR
./regress-pair-init -W $WOOF -s $SIZE -c $COUNTBACK
cd $HERE

STARTTS=`tail -n $COUNTBACK $STATIONFILE | head -n 1 | awk '{print $1}'`
echo start TS $STARTTS

if ( test -z "$STARTTS" ) ; then
	echo "no stattion file $STATIONFILE found"
	exit 1
fi


ESEQNO=`$BIN/senspot-get -W $EXPWOOF | awk '{print $6}'`

if ( test -z "$ESEQNO" ) ; then
	echo "can't get last seqno from $EXPWOOF"
	exit 1
fi

if ( test $ESEQNO -le 0 ) ; then
	echo "seqno $ESEQNO invalid from $EXPWOOF"
	exit 1
fi

SEQNO=$ESEQNO
echo starting seqno $ESEQNO for $EXPWOOF
while ( test $SEQNO -gt 0 ) ; do
	NEXTS=`$BIN/senspot-get -W $EXPWOOF -S $SEQNO | awk '{print $3}' | awk -F '.' '{print $1}'`
echo $NEXTS $SEQNO
	if ( test -z "$NEXTS" ) ; then
		echo "bad TS $NEXTS in $EXPWOOF at seqno $SEQNO"
		exit 1
	fi
	if ( test $NEXTS -le $STARTTS ) ; then
		break
	fi
	SEQNO=$(($SEQNO-1))
done

echo "starting TS in $STATIONFILE is $STARTS and starting TS in $EXPWOOF at $SEQNO is $NEXTS"

while ( test $SEQNO -le $ESEQNO ) ; do
	LINE=`$BIN/senspot-get -W $EXPWOOF -S $SEQNO`
	TS=`echo $LINE | awk '{print $3}'`
	VALUE=`echo $LINE | awk '{print $1}'`
	echo "priming $WOOF with measured $TS $VALUE"
	echo $TS $VALUE | $BIN/regress-pair-put -W $WOOF -s 'm'
	sleep 0.05
	SEQNO=$(($SEQNO+1))
done
	
CNT=$COUNTBACK
while ( test $CNT -gt 0 ) ; do
	LINE=`tail -n $CNT $STATIONFILE | head -n 1`
	echo "priming $WOOF with predicted $LINE"
	echo $LINE | $BIN/regress-pair-put -W $WOOF -s 'p'
	sleep 0.05
	CNT=$(($CNT-1))
done
