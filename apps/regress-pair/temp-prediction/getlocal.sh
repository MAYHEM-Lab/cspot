#! /bin/bash
if [ "$#" -ne 8 ]; then
    echo "USAGE: ./getlocal.sh -d DAYS -s STATION -o OUTFILE -a APIKEY"
    echo "OUTFILE will be deleted prior to usage."
    echo "You may need to run \"sudo pip install requests; sudo pip2.7 install requests\""
    exit 1
fi

while getopts d:s:a:o: option
do
 case "${option}"
 in
 d) DAYS="$((OPTARG))";;
 s) STATION="${OPTARG}";;
 a) APIKEY="${OPTARG}";;
 o) OUTF="${OPTARG}";;
 esac
done

#echo days: $DAYS
#echo station: $STATION
#echo apikey: $APIKEY
#echo output file: $OUTF

rm -f ${OUTF}
unamestr=`uname`
val=1
if [[ "$unamestr" == 'Darwin' ]]; then
   #sstr="-v-1d"
   #ENDDATE=`date ${sstr} +%Y-%m-%d`
   ENDDATE=`date +%Y-%m-%d`
else
   #ENDDATE=$(date -d "-1 days" +%Y-%m-%d)
   ENDDATE=$(date +%Y-%m-%d)
fi 
#echo enddate: $ENDDATE
if [[ "$unamestr" == 'Darwin' ]]; then
   sstr="-v-${DAYS}d"
   STARTDATE=`date ${sstr} +%Y-%m-%d`
else
   STARTDATE=$(date -d "-${DAYS} days" +%Y-%m-%d)
fi 
#echo startdate: $STARTDATE
echo "Requesting data from CIMIS for the last ${DAYS} days and station ${STATION}... (please wait)"
#python cimis.py 2018-01-10 2018-01-16 0e1a4d1a-347c-40af-b811-a14958a8eba8 --hourly_wsn out.txt --station 80
python /smartedge/bin/cimis.py ${STARTDATE} ${ENDDATE} ${APIKEY} --hourly_wsn ${OUTF} --station ${STATION}
echo "Your data is ready in file ${OUTF}."
echo "If all values are -1, check that your APIKEY is valid."
