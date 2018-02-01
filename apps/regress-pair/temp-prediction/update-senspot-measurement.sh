#/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

BIN=/smartedge/bin

WOOF=$1
SWOOF=$2

if ( test -z "$WOOF" ) ; then
	echo "update-senspot-measurement.sh woof source-woof"
	exit 1
fi

if ( test -z "$SWOOF" ) ; then
	echo "update-senspot-measurement woof source-woof"
	exit 1
fi

LASTSEQNO=`$BIN/regress-pair-get -W $WOOF -s 'm' | awk '{print $5}'`
LAST=`$BIN/regress-pair-get -W $WOOF -s 'm' | awk '{print $3}' | awk -F '.' '{print $1}'`
if ( test "$LASTSEQNO" = "seq_no:" ) ; then
	LASTSEQNO=`$BIN/regress-pair-get -W $WOOF -s 'm' | awk '{print $6}'`
fi

SEQNO=`$BIN/senspot-get -W $SWOOF | awk '{print $6}'`

CNT=0
while ( test $CNT -lt 500 ) ; do
	LINE=`$BIN/senspot-get -W $SWOOF -S $(($SEQNO - $CNT))`
	LASTM=`echo $LINE | awk '{print $3}' | awk -F '.' '{print $1}'`
	if ( test $LASTSEQNO -gt 0 ) ; then
		if ( test $LASTM -gt $LAST ) ; then
			break
		fi
		echo "updating $LASTM $MEAS (seqno $(($SEQNO - $CNT)))"
	else
		echo "initial put of $LASTM $MEAS (seqno $(($SEQNO - $CNT)))"
	fi
	MEAS=`echo $LINE | awk '{print $1}'`
	echo $LASTM $MEAS | $BIN/regress-pair-put -W $WOOF -s 'm'
	CNT=$(($CNT+1))
done


