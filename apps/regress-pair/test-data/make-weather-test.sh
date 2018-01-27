#!/bin/bash

COUNTBACK=5

DAY3=$((3600*$COUNTBACK))
CIMAS=$1
NUC=$2
WOOF=$3

if ( test -z "$CIMAS" ) ; then
	echo "make-weather-test cimas-file nuc-file woof-name"
	exit 1
fi

if ( test -z "$NUC" ) ; then
	echo "make-weather-test cimas-file nuc-file woof-name"
	exit 1
fi

if ( test -z "$WOOF" ) ; then
	echo "make-weather-test cimas-file nuc-file woof-name"
	exit 1
fi

./regress-pair-init -W $WOOF -c $COUNTBACK -s 500

FIRSTNUCTS=`head -n 1 $NUC | awk '{print $1}' | sed 's/\./ /' | awk '{print $1}'`

echo NUC

for NUCTS in `awk '{print $1}' $NUC | sed 's/\./ /' | awk '{print $1}'` ; do
	if ( test $NUCTS -gt $(($FIRSTNUCTS+$DAY3)) ) ; then
		break
	fi
	LINE=`grep $NUCTS $NUC`
	echo $LINE | ./regress-pair-put -W $WOOF -L -s 'm'
done

echo CIMAS

for CIMASTS in `awk '{print $1}' $CIMAS | sed 's/\./ /' | awk '{print $1}'` ; do
	if ( test $CIMASTS -gt $(($FIRSTNUCTS+$DAY3)) ) ; then
		break
	fi
	LINE=`grep $CIMASTS $CIMAS`
	echo $LINE | ./regress-pair-put -W $WOOF -L -s 'p'
done

for NUCTS in `awk '{print $1}' $NUC | sed 's/\./ /' | awk '{print $1}'` ; do
	if ( test $NUCTS -le $(($FIRSTNUCTS+$DAY3)) ) ; then
		continue
	fi
	if ( test $NUCTS -gt $(($FIRSTNUCTS+DAY3+3600)) ) ; then
		break
	fi
	LINE=`grep $NUCTS $NUC`
	echo $LINE | ./regress-pair-put -W $WOOF -L -s 'm'
done

for CIMASTS in `awk '{print $1}' $CIMAS | sed 's/\./ /' | awk '{print $1}'` ; do
	if ( test $CIMASTS -le $(($FIRSTNUCTS+$DAY3)) ) ; then
		continue
	fi
	if ( test $CIMASTS -gt $(($FIRSTNUCTS+$DAY3+3600)) ) ; then
		break
	fi
	LINE=`grep $CIMASTS $CIMAS`
	echo $LINE | ./regress-pair-put -W $WOOF -L -s 'p'
done


