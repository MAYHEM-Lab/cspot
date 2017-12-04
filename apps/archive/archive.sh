#!/bin/bash

HERE=/mnt/edge-data

BIN=/root/bin

$BIN/read-archive.sh woof://128.111.45.83/mnt/monitor/tin-shed.temp $HERE/tin-shed.temp.txt
$BIN/read-archive.sh woof://128.111.45.83/mnt/monitor/eduroam.upload $HERE/eduroam.upload.txt
$BIN/read-archive.sh woof://128.111.45.83/mnt/monitor/eduroam.download $HERE/eduroam.download.txt
$BIN/read-archive.sh woof://128.111.45.83/mnt/monitor/home.download $HERE/home.download.txt
$BIN/read-archive.sh woof://128.111.45.83/mnt/monitor/home.upload $HERE/home.upload.txt
$BIN/read-archive.sh woof://128.111.45.83/mnt/monitor/office.download.wired $HERE/office.download.wired.txt
$BIN/read-archive.sh woof://128.111.45.83/mnt/monitor/office.upload.wired $HERE/office.upload.wired.txt
$HERE/make-archive-html.sh
