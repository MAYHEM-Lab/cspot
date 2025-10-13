#!/bin/bash -f
cp ../../apps/self-test/latency.sh .
cp ../../apps/self-test/throughput.sh .
#echo "$(pwd)"
$(pwd)/woofc-namespace-platform -b spawn >& namespace.log &
WPID=`ps auxww | grep "$(pwd)/woofc-namespace-platform" | grep -v grep | awk '{print $2}'`
#CPID=`ps auxww | grep "$(pwd)/woofc-container" | grep -v grep | awk '{print $2}'`
#WLIST=`ps auxww | grep "$(pwd)/woofc-forker-helper" | grep -v grep | awk '{print $2}'`
#echo $WPID
LTEST=`./latency.sh 5 | grep "avg latency" | wc -l | awk '{print $1}'`
if ( test $LTEST -eq 5 ) ; then
	echo "SELF TEST 1 PASSED"
else
	echo "SELF TEST 1 FAILED"
	rm -f zzzstress
	kill -HUP $WPID
	exit 1
fi
LTEST=`./throughput.sh 100 | grep "seq_no" | wc -l | awk '{print $1}'`
if ( test $LTEST -eq 100 ) ; then
	echo "SELF TEST 2 PASSED"
else
	echo "SELF TEST 2 FAILED"
	rm -f zzzstress
	kill -HUP $WPID
	exit 1
fi
rm -f zzzstress
#echo "sending HUP to $WPID"
kill -HUP $WPID
#kill -HUP $WLIST
#kill -HUP $CPID
exit 0


