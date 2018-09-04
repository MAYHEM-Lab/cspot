#!/bin/bash

WOOF=$1

BIN=/Users/rich/bin

if ( test -z "$WOOF" ) ; then
	echo "temp-get.sh woof-name"
	exit 1
fi

LAST_LINE=`$BIN/regress-pair-get -W $WOOF -s 'e'`
LAST_E_TS=`echo $LAST_LINE | awk '{print $3}'`
LAST_E_SEQNO=`echo $LAST_LINE | awk '{print $6}'`

ECNT=1
PCNT=1
PLINE=`$BIN/regress-pair-get -W $WOOF -s 'p' -S 1`
P_TS=`echo $PLINE | awk '{print $3}' | awk -F '.' '{print $1}'`
while ( test $ECNT -le $LAST_E_SEQNO ) ; do
	ELINE=`$BIN/regress-pair-get -W $WOOF -s 'e' -S $ECNT`
	E_TS=`echo $ELINE | awk '{print $3}' | awk -F '.' '{print $1}'`
	while ( test $P_TS -lt $E_TS ) ; do
		PCNT=$(($PCNT+1))
		PLINE=`$BIN/regress-pair-get -W $WOOF -s 'p' -S $PCNT`
		if ( test -z "$PLINE" ) ; then
			break
		fi
		P_TS=`echo $PLINE | awk '{print $3}' | awk -F '.' '{print $1}'`
	done
	ERR=`echo $ELINE | awk '{print $1}'`
	PRED=`echo $PLINE | awk '{print $1}'`
	if ( test $P_TS -eq $E_TS ) ; then
		echo "$E_TS $PRED $ERR"
	fi
	ECNT=$(($ECNT+1))
done
		
	


