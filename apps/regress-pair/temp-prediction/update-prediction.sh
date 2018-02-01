#/bin/bash

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

BIN=/smartedge/bin

WOOF=$1
FILE=$2

if ( test -z "$WOOF" ) ; then
	echo "update-prediction woof file"
	exit 1
fi

if ( test -z "$FILE" ) ; then
	echo "update-prediction woof file"
	exit 1
fi

LASTTIME=`$BIN/regress-pair-get -W $WOOF -s 'p' | awk '{print $3}' | awk -F '.' '{print $1}'`

for LASTP in `tail -n 500 $FILE | awk '{print $1}'` ; do
	if ( test "$LASTTIME" = "0" ) ; then
		LINE=`grep $LASTP $FILE | tail -n 1`
		echo $LINE | $BIN/regress-pair-put -W $WOOF -s 'p'
		sleep 0.1
		continue
	fi
	if ( test $LASTP -gt $LASTTIME ) ; then
		LINE=`grep $LASTP $FILE | tail -n 1`
		echo $LINE | $BIN/regress-pair-put -W $WOOF -s 'p'
		sleep 0.1
	fi
done


