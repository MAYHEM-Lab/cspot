#!/bin/bash

HERE=/mnt/edge-data

awk '{print $3,$1}' $HERE/$1 | tail -n 2016 > $HERE/zzzplotly
FILE=$HERE/zzzplotly

echo "<head>"
echo "<script src=\"https://cdnjs.cloudflare.com/ajax/libs/d3/3.5.6/d3.min.js\"></script>"
echo "<script src=\"https://code.jquery.com/jquery-2.1.4.min.js\"></script>"
echo "<script src=\"https://d14fo0winaifog.cloudfront.net/plotly-basic.js\"></script>"
echo "</head>"
echo "<body>"
DATE=`/bin/date`
echo "<h3>Graph Generated on " $DATE "</h3>"
echo "<p>"
echo "<div id=\"myDiv\"></div>"
echo "<script>"
echo "var trace1 = {"
DATAX=`awk '{print $1}' $FILE | sed 's/$/,/g'`
XVALS="x: [$DATAX]"
XVALS=`echo $XVALS | sed 's/,]/]/'`
echo $XVALS","
DATAY=`awk '{print $2}' $FILE | sed 's/$/,/g'`
YVALS="y: [$DATAY]"
YVALS=`echo $YVALS | sed 's/,]/]/'`
echo $YVALS","
echo "mode: 'lines+markers',"
echo "type: 'scatter'"
echo "};"
echo "var data = [trace1];"
echo "var layout = {"
echo "title: 'Last Days Plot',"
echo "xaxis: {"
echo "title: 'Time',"
echo "titlefont: {"
echo "family: 'Courier New, monospace',"
echo "size: 14,"
echo "color: '#7f7f7f'"
echo "}"
echo "},"
echo "yaxis: {"
echo "title: 'Measurements',"
echo "titlefont: {"
echo "family: 'Courier New, monospace',"
echo "size: 12,"
echo "color: '#7f7f7f'"
echo "}"
echo "}"
echo "};"
echo "Plotly.newPlot('myDiv', data,layout);"
echo "</script>"
echo "</body>"
echo "</html>"

