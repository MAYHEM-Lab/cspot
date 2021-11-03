#!/bin/bash

HERE=/mnt/edge-data/fresnostate
HTML=/mnt/html/fresnostate

rm -rf $HERE/index.html
for FILE in `/usr/bin/ls $HERE/*.txt` ; do
	FROOT=`echo $FILE | sed 's$/$ $g' | awk '{print $4}'`
	cp $HERE/$FROOT $HTML/$FROOT
	chmod 644 $HTML/$FROOT
	$HERE/make-regress-plotly.sh $FROOT > $HTML/$FROOT-7.html
	$HERE/make-regress-errors-plotly.sh $FROOT > $HTML/$FROOT-ERR-7.html
	chmod 644 $HTML/$FROOT-7.html
	echo '<a href=' $FROOT '>' $FROOT '</a> <a href=' $FROOT-7.html '>(data last seven days)</a> <a href=' $FROOT-ERR-7.html '>(errors last seven days)' >> $HERE/index.html
	echo "<p>" >> $HERE/index.html
done
cp $HERE/index.html $HTML/index.html
chmod 644 $HTML/index.html


