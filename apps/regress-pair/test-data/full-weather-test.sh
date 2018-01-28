#!/bin/bash

COUNTBACK=24

CBTIME=$((3600*$COUNTBACK))
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

./regress-pair-init -W $WOOF -c $COUNTBACK -s 120000

FIRSTNUCTS=`head -n 1 $NUC | awk '{print $1}' | sed 's/\./ /' | awk '{print $1}'`
LASTNUCTS=`tail -n 1 $NUC | awk '{print $1}' | sed 's/\./ /' | awk '{print $1}'`
CURRTS=$FIRSTNUCTS


# prime the pump
for NUCTS in `awk '{print $1}' $NUC | sed 's/\./ /' | awk '{print $1}'` ; do
	if ( test $NUCTS -gt $(($FIRSTNUCTS+$CBTIME)) ) ; then
		break
	fi
	LINE=`grep $NUCTS $NUC`
	echo $LINE | ./regress-pair-put -W $WOOF -L -s 'm'
	CURRTS=$NUCTS
	sleep 0.1
done


for CIMASTS in `awk '{print $1}' $CIMAS | sed 's/\./ /' | awk '{print $1}'` ; do
	if ( test $CIMASTS -gt $(($FIRSTNUCTS+$CBTIME)) ) ; then
		break
	fi
	LINE=`grep $CIMASTS $CIMAS`
	echo $LINE | ./regress-pair-put -W $WOOF -L -s 'p'
	sleep 0.1
done

CNT=1
while ( test $CURRTS -le $LASTNUCTS ) ; do

	for NUCTS in `awk '{print $1}' $NUC | sed 's/\./ /' | awk '{print $1}'` ; do

		if ( test $NUCTS -le $(($FIRSTNUCTS+$CBTIME+($CNT*86400))) ) ; then
			continue
		fi
		if ( test $NUCTS -gt $(($FIRSTNUCTS+CBTIME+(($CNT+1)*86400))) ) ; then
			break
		fi
		LINE=`grep $NUCTS $NUC`
		echo $LINE | ./regress-pair-put -W $WOOF -L -s 'm'
		CURRTS=$NUCTS
		sleep 0.1
	done

	for CIMASTS in `awk '{print $1}' $CIMAS | sed 's/\./ /' | awk '{print $1}'` ; do
		if ( test $CIMASTS -le $(($FIRSTNUCTS+$CBTIME+($CNT*86400))) ) ; then
			continue
		fi
		if ( test $CIMASTS -gt $(($FIRSTNUCTS+$CBTIME+(($CNT+1)*86400))) ) ; then
			break
		fi
		LINE=`grep $CIMASTS $CIMAS`
		echo $LINE | ./regress-pair-put -W $WOOF -L -s 'p'
		sleep 0.1
	done
	CNT=$(($CNT+1))

done

