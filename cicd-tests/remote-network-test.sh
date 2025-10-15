#!/bin/bash -f
ADDR=169.231.231.109
cp ../../apps/self-test/latency-net.sh .
cp ../../apps/self-test/throughput-net.sh .
#echo "$(pwd)"
$(pwd)/start-remote-platform.sh
ssh $ADDR "cd /home/ubuntu/cspot/build/bin && ./stress-init -W zzzstress -s 100"
LTEST=`./latency-net.sh $ADDR 5 /home/ubuntu/cspot/build/bin | grep "avg latency" | wc -l | awk '{print $1}'`
if ( test $LTEST -eq 5 ) ; then
	echo "SELF TEST REMOTE 1 PASSED"
else
	echo "SELF TEST REMOTE 1 FAILED"
	rm -f zzzstress
	kill -HUP $WPID
	exit 1
fi
LTEST=`./throughput-net.sh $ADDR 100 /home/ubuntu/cspot/build/bin | grep "seq_no" | wc -l | awk '{print $1}'`
if ( test $LTEST -eq 100 ) ; then
	echo "SELF TEST REMOTE 2 PASSED"
else
	echo "SELF TEST REMOTE 2 FAILED"
	rm -f zzzstress
	kill -HUP $WPID
	exit 1
fi
$(pwd)/kill-remote-platform.sh 
ssh $ADDR "rm -f /home/ubuntu/cspot/build/bin/zzzstress*"
#echo "sending HUP to $WPID"
#
kill -HUP $WPID

exit 0


