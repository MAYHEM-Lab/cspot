#!/bin/bash

SIZE=$((1024*100))

while ( test $SIZE -le $((1024*1024*100)) ) ; do
	./bw.sh 100 $SIZE
	SIZE=$(($SIZE + (100*1024)))
done

