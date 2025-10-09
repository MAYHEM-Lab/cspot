#!/bin/bash
cp ../../apps/self-test/latency.sh .
cp ../../apps/self-test/throughput.sh .
./woofc-namespace-platform >& namespace.log &
LTEST=`./latency.sh 5 | grep "avg latency" | wc -l | awk '{print $1}'`
if ( test $LTEST -eq 5 ) ; then
	echo "SELF TEST 1 PASSED"
else
	echo "SELF TEST 1 FAILED"
fi
LTEST=`./throughput.sh 100 | grep "seq_no" | wc -l | awk '{print $1}'`
if ( test $LTEST -eq 100 ) ; then
	echo "SELF TEST 2 PASSED"
else
	echo "SELF TEST 2 FAILED"
fi
rm -f zzzstress
kill -HUP %1


