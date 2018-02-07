#!/bin/bash

BIN=/smartedge/bin


# 2013-02-06 21:04:15

DSTR=$1
TSTR=$2

MONTH=`echo $DSTR | sed 's$/$ $g' | awk '{print $1}'`
DAY=`echo $DSTR | sed 's$/$ $g' | awk '{print $2}'`
YEAR=`echo $DSTR | sed 's$/$ $g' | awk '{print $3}'`
YEAR="20"$YEAR

if ( test "$TSTR" = "0000" ) ; then
	TS=0
elif ( test "$TSTR" = "0100" ) ; then
	TS=100
elif ( test "$TSTR" = "0200" ) ; then
	TS=200
elif ( test "$TSTR" = "0300" ) ; then
	TS=300
elif ( test "$TSTR" = "0400" ) ; then
	TS=400
elif ( test "$TSTR" = "0500" ) ; then
	TS=500
elif ( test "$TSTR" = "0600" ) ; then
	TS=600
elif ( test "$TSTR" = "0700" ) ; then
	TS=700
elif ( test "$TSTR" = "0800" ) ; then
	TS=800
elif ( test "$TSTR" = "0900" ) ; then
	TS=900
else
	TS=$TSTR
fi


HR=$(($TS / 100))
HMIN=$(($HR * 100))
MIN=$(($TS - $HMIN))

#echo "$YEAR-$MONTH-$DAY $HR:$MIN:00"
echo "$DSTR $HR:$MIN:00"

