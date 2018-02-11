#/bin/bash

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

BIN=/mnt/test

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

for LASTP in `tail -n 1000 $FILE | awk '{print $1}'` ; do
	if ( test "$LASTTIME" = "0" ) ; then
		LINE=`grep $LASTP $FILE | tail -n 1`
		echo $LINE | $BIN/regress-pair-put -W $WOOF -s 'p'
		continue
	fi
	if ( test -z "$LASTTIME" ) ; then
		LINE=`grep $LASTP $FILE | tail -n 1`
echo "empty put of $LINE"
		echo $LINE | $BIN/regress-pair-put -W $WOOF -s 'p'
		continue
	fi
	if ( test $LASTP -gt $LASTTIME ) ; then
		LINE=`grep $LASTP $FILE | tail -n 1`
		TEMP=`echo $LINE | awk '{print $2}'`
		if ( test "$TEMP" = "-999" ) ; then
			continue
		fi
		if ( test "$TEMP" = "-1" ) ; then
			continue
		fi
echo "update put of $LINE"
		echo $LINE | $BIN/regress-pair-put -W $WOOF -s 'p'
	fi
done


