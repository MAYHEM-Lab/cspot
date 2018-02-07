#!/bin/bash

BIN=/smartedge/bin

STATION=$1
DAYS=$2
OUTFILE=$3

if ( test -z "$STATION" ) ; then
	echo "get-cimas.sh station days output_file"
	exit 1
fi
if ( test -z "$DAYS" ) ; then
	echo "get-cimas.sh station days output_file"
	exit 1
fi
if ( test -z "$OUTFILE" ) ; then
	echo "get-cimas.sh station days output_file"
	exit 1
fi

rm -f $OUTFILE.raw

if ( ! test -e "$OUTFILE" ) ; then
	touch $OUTFILE
fi

#$BIN/getlocal.sh -d $DAYS -s $STATION -o $OUTFILE.raw -a "0e1a4d1a-347c-40af-b811-a14958a8eba8"
$BIN/getlocal.sh -d $DAYS -s $STATION -o $OUTFILE.raw -a "149aabdc-591e-4344-9780-bdb1e435726f"


CNT=2
SIZE=`wc -l $OUTFILE.raw | awk '{print $1}'`

while ( test $CNT -le $SIZE ) ; do
        LINE=`head -n $CNT $OUTFILE.raw | tail -n 1`
	if ( test -z "$LINE" ) ; then
        	CNT=$(($CNT+1))
		continue
	fi
        DSTR=`echo $LINE | awk -F ',' '{print $1}'`
        TSTR=`echo $LINE | awk -F ',' '{print $3}'`
        TS=`$BIN/time-change.sh $DSTR $TSTR | $BIN/convert_time`
        TEMP=`echo $LINE | awk -F ',' '{print $4}'`
	if ( test "$TS" != "-1" ) ; then
		if ( test "$TEMP" != "-1" ) ; then
			TSTEST=`grep $TS $OUTFILE`
			if ( test -z "$TSTEST" ) ; then
        			echo $TS $TEMP >> $OUTFILE
			fi
		fi
	fi
        CNT=$(($CNT+1))
done



