#!/bin/bash -f
cp ../../apps/self-test/latency-net.sh .
cp ../../apps/self-test/throughput-net.sh .
#echo "$(pwd)"
$(pwd)/woofc-namespace-platform -b spawn >& namespace.log &
WPID=`ps auxww | grep "$(pwd)/woofc-namespace-platform" | grep -v grep | awk '{print $2}'`
#echo $WPID
LTEST=`./latency-net.sh 127.0.0.1 5 | grep "avg latency" | wc -l | awk '{print $1}'`
if ( test $LTEST -eq 5 ) ; then
	echo "SELF TEST 1 PASSED"
else
	echo "SELF TEST 1 FAILED"
	rm -f zzzstress
	kill -HUP $WPID
	exit 1
fi
LTEST=`./throughput-net.sh 127.0.0.1 100 | grep "seq_no" | wc -l | awk '{print $1}'`
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
exit 0


