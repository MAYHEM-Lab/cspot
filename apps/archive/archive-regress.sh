#!/bin/bash

HERE=/mnt/edge-data/fresnostate

BIN=/root/bin

$BIN/read-archive-regress.sh woof://169.231.234.231/smartedge/fresnostate/nuc-wu $HERE/nuc-wu59.txt
$BIN/read-archive-regress.sh woof://169.231.234.231/smartedge/fresnostate/nuc-wu30 $HERE/nuc-wu30.txt
$BIN/read-archive-regress.sh woof://169.231.234.231/smartedge/fresnostate/nuc-wu50 $HERE/nuc-wu50.txt
$BIN/read-archive-regress.sh woof://169.231.234.231/smartedge/fresnostate/nuc-cimis80 $HERE/nuc-cimis80.txt

$HERE/make-regress-archive-html.sh
