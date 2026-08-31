#!/bin/bash -f
cp ../../apps/self-test/latency.sh .
cp ../../apps/self-test/throughput.sh .
echo "local-self-test $(pwd)"
cd $(pwd)
WPID=`ps auxww | grep actions | grep  "woofc" | grep -v grep | awk '{print $2}'`
if ( ! test -z "$WPID" ) ; then
	kill -9 $WPID
fi
./woofc-namespace-platform -b spawn >& namespace.log &
RTEST=`cat ./namespace.log | grep config`
CNT=0
while ( test $CNT -lt 10 ) ; do
	if ( ! test -z "$RTEST" ) ; then
		break
	fi
	sleep 1
	RTEST=`cat ./namespace.log | grep config`
	CNT=$(($CNT+1))
done
if ( test $CNT -ge 10 ) ; then
	echo "local-self-test could not start namespace server"
	exit 1
fi
WPID=`ps auxww | grep actions | grep  "woofc" | grep -v grep | awk '{print $2}'`
#CPID=`ps auxww | grep "$(pwd)/woofc-container" | grep -v grep | awk '{print $2}'`
#WLIST=`ps auxww | grep "$(pwd)/woofc-forker-helper" | grep -v grep | awk '{print $2}'`
#echo $WPID
LTEST=`./latency.sh 5 | grep "avg latency" | wc -l | awk '{print $1}'`
if ( test $LTEST -eq 5 ) ; then
	echo "SELF TEST 1 PASSED"
else
	echo "SELF TEST 1 FAILED"
	rm -f zzzstress
	kill -9 $WPID
	exit 1
fi
LTEST=`./throughput.sh 100 | grep "seq_no" | wc -l | awk '{print $1}'`
if ( test $LTEST -eq 100 ) ; then
	echo "SELF TEST 2 PASSED"
else
	echo "SELF TEST 2 FAILED"
	rm -f zzzstress
	kill -9 $WPID
	exit 1
fi
rm -f zzzstress
#echo "sending HUP to $WPID"
kill -9 $WPID
#kill -HUP $WLIST
#kill -9 $CPID
exit 0


