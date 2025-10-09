#!/bin/bash
cp ../../apps/self-test/latency.sh .
cp ../../apps/self-test/throughput.sh .
./woofc-namespace-platform -b spawn >& namespace.log &
LTEST=`./latency.sh 5 | grep "avg latency" | wc -l | awk '{print $1}'`
if ( test $LTEST -eq 5 ) ; then
	echo "SELF TEST 1 PASSED"
else
	echo "SELF TEST 1 FAILED"
	rm -f zzzstress
	kill -HUP %1
	exit 1
fi
LTEST=`./throughput.sh 100 | grep "seq_no" | wc -l | awk '{print $1}'`
if ( test $LTEST -eq 100 ) ; then
	echo "SELF TEST 2 PASSED"
else
	echo "SELF TEST 2 FAILED"
	rm -f zzzstress
	kill -HUP %1
	exit 1
fi
cat namespace.log
rm -f zzzstress
kill -9 `ps auxww | grep woofc | grep actions-runner | grep "_work" | grep -v grep | awk '{print $2}'`
kill -HUP %1
exit 0


