#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

HERE=`pwd`

if ( test -z "$1" ) ; then
	echo "read-temp.sh must specify target woof name"
	exit 1
fi

WNAME=$1

if ( test -z "$2" ) ; then
	READINGS=1
else
	READINGS=$2
fi

CNT=0

UN=`uname`
if ( test "$UN" = "Linux" ) ; then
	ISLINUX=T
else
	ISLINUX=
fi

while ( test $CNT -lt $READINGS ) ; do
	if ( test -z "$SEQNO" ) ; then
		LINE=`$HERE/senspot-get -W $WNAME`
		SEQNO=`echo $LINE | awk '{print $6}'`
		TIME=`echo $LINE | awk '{print $3}' | sed 's/\./ /' | awk '{print $1}'`
		TSIZE=`echo $TIME | wc | awk '{print $3}'`
		if ( test -z "$ISLINUX" ) ; then
			ODATE=`date`
			NOW=`date -j -f "%a %b %d %T %Z %Y" "$ODATE" +"%s"`
			NSIZE=`echo $NOW | wc | awk '{print $3}'`
                        if( test $TSIZE -ne $NSIZE ) ; then
                                DT=`date -r $NOW`
				NLINE=`echo ${LINE/$TIME/$NOW}`
				LINE=$NLINE
                        else
                                DT=`date -r $TIME`
                        fi

		else
			NOW=`date "+%s"`
			NSIZE=`echo $NOW | wc | awk '{print $3}'`
                        if ( test $TSIZE -ne $NSIZE ) ; then
                                DT=`date --date @$NOW`
				NLINE=`echo ${LINE/$TIME/$NOW}`
				LINE=$NLINE
                        else
                                DT=`date --date @$TIME`
                        fi
		fi
		echo $LINE "--" $DT
	else
		SEQNO=$(($SEQNO-1))
		if( test $SEQNO -le 0 ) ; then
			break
		fi
		LINE=`$HERE/senspot-get -W $WNAME -S $SEQNO`
		TIME=`echo $LINE | awk '{print $3}' | sed 's/\./ /' | awk '{print $1}'`
		TSIZE=`echo $TIME | wc | awk '{print $3}'`
		if ( test -z "$ISLINUX" ) ; then
			ODATE=`date`
			NOW=`date -j -f "%a %b %d %T %Z %Y" "$ODATE" +"%s"`
			NSIZE=`echo $NOW | wc | awk '{print $3}'`
                        if( test $TSIZE -ne $NSIZE ) ; then
                                DT=`date -r $NOW`
				NLINE=`echo ${LINE/$TIME/$NOW}`
				LINE=$NLINE
                        else
                                DT=`date -r $TIME`
                        fi
		else
			NOW=`date "+%s"`
			NSIZE=`echo $NOW | wc | awk '{print $3}'`
                        if ( test $TSIZE -ne $NSIZE ) ; then
                                DT=`date --date @$NOW`
				NLINE=`echo ${LINE/$TIME/$NOW}`
				LINE=$NLINE
                        else
                                DT=`date --date @$TIME`
                        fi
		fi
		echo $LINE "--" $DT
	fi
	CNT=$(($CNT+1))
done
		
		
		

