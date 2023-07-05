#!/bin/bash

for SZ in 2 5 10 15 20 25 30 35 40 45 50 55 60 65 70 75 80 85 90 95 100 ; do
	./mm_init -d $SZ
	OUT=""
	while ( test -z "$OUT" ) ; do
		MTEST=`ps auxww | grep "MM_" | grep -v grep`
		while ( ! test -z "$MTEST" ) ; do
			sleep 10
			MTEST=`ps auxww | grep "MM_" | grep -v grep`
		done
		OUT=`./mm_print_result | grep total`
	done
	echo $SZ $OUT
done

