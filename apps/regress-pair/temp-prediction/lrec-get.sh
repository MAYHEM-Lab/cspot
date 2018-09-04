#!/bin/bash


FILE=$1
#
if ( test -z "$FILE" ) ; then
        echo "lrec-get file"
        exit 1
fi

#JSON=`wget -O - http://api.wunderground.com/api/$KEY/conditions/q/pws:$STATION.json`

# 03/09/2018 16:44:15

DSTR=`wget -O - http://166.140.25.70:5682 | sed 's/&nbsp/ /g' | sed 's/;/ /g' | sed 's/<br>/\n/g' | sed 's/<p>/\n/g' | grep "Date" | awk '{print $2,$3}'`
DS=`echo $DSTR | sed 's$/$ $g' | awk '{print $3"-"$1"-"$2}'`
TV=`echo $DSTR | awk '{print $2}'`
TS=`echo $DS $TV | /root/bin/convert_time`

TEMP=`wget -O - http://166.140.25.70:5682 | sed 's/&nbsp/ /g' | sed 's/;/ /g' | sed 's/<br>/\n/g' | grep 'Air Temp 5ft:' | awk '{printf "%2.1f", $4}'`

if ( ! test -e "$FILE" ) ; then
        echo $TS $TEMP > $FILE
        exit 0
fi
LAST_TS=`tail -n 1 $FILE | awk '{print $1}'`

if ( test $TS -le $LAST_TS ) ; then
        exit 0
fi

if ( ! test -z "$TS" ) ; then
        echo $TS $TEMP >> $FILE
fi

exit 0

