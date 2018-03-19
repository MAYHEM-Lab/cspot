#/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

BIN=/smartedge/bin

WOOF=$1
SWOOF=$2

if ( test -z "$WOOF" ) ; then
	echo "update-dht22-measurement.sh woof source-woof"
	exit 1
fi

if ( test -z "$SWOOF" ) ; then
	echo "update-dht22-measurement woof source-woof"
	exit 1
fi

LAST=`$BIN/regress-pair-get -W $WOOF -s 'p' | awk '{print $3}' | awk -F '.' '{print $1}'`

LASTSEQNO=`$BIN/regress-pair-get -W $WOOF -s 'p' | awk '{print $6}'`


if ( test "$LASTSEQNO" = "seq_no:" ) ; then
	LASTSEQNO=`$BIN/regress-pair-get -W $WOOF -s 'p' | awk '{print $6}'`
fi
if ( test "$LASTSEQNO" = " seq_no:" ) ; then
	LASTSEQNO=`$BIN/regress-pair-get -W $WOOF -s 'p' | awk '{print $6}'`
fi

if ( test "$LASTSEQNO" = "seq_no: " ) ; then
	LASTSEQNO=`$BIN/regress-pair-get -W $WOOF -s 'p' | awk '{print $6}'`
fi

if ( test -z "$LASTSEQNO" ) ; then
	LASTSEQNO=0
fi

SEQNO=`$BIN/senspot-get -W $SWOOF | awk '{print $6}'`

CNT=0
OFFSET=100
while ( test $OFFSET -ge 0 ) ; do
	LINE=`$BIN/senspot-get -W $SWOOF -S $(($SEQNO - $OFFSET))`
	LASTM=`echo $LINE | awk '{print $3}' | awk -F '.' '{print $1}'`
	if ( test $LASTSEQNO -gt 0 ) ; then
		if ( test $LASTM -le $LAST ) ; then
			CNT=$(($CNT+1))
			OFFSET=$(($OFFSET-1))
			continue
		fi
		MEAS=`echo $LINE | awk '{print $1}' | awk -F ':' '{print $1}'`
		echo "updating $LASTM $MEAS (seqno $(($SEQNO - $OFFSET)))"
	else
		MEAS=`echo $LINE | awk '{print $1}' | awk -F ':' '{print $1}'`
		echo "initial put of $LASTM $MEAS (seqno $(($SEQNO - $OFFSET)))"
	fi
	MEAS=`echo $LINE | awk '{print $1}'`
	if ( ! test -z "$MEAS" ) ; then
		echo $LASTM $MEAS | $BIN/regress-pair-put -W $WOOF -s 'p'
#		sleep 0.1
	fi
	CNT=$(($CNT+1))
	OFFSET=$(($OFFSET-1))
done


