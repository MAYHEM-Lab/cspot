#!/bin/bash -f
#
source ../../../../../cicd-config.sh
ADDR=$X86HELPER
$(pwd)/start-remote-platform.sh
ssh ubuntu@$ADDR "cd /home/ubuntu/cspot/build/bin && ./senspot-init -W zzzsenspot -s 100"
echo "zzzsenspot created"
LTEST=`echo "3.1415" | ./senspot-put -W woof://$ADDR/home/ubuntu/cspot/build/bin/zzzsenspot -T d`
if ( test -z "$LTEST" ) ; then
	echo "SELF TEST SENSPOT PUT PASSED"
else
	echo "SELF TEST SENSPOT PUT FAILED $LTEST"
	exit 1
fi
MTEST=`./senspot-get -W woof://$ADDR/home/ubuntu/cspot/build/bin/zzzsenspot | awk '{print $1}'`
if ( test "$MTEST" == "3.141500" ) ; then
	echo "SELF TEST SENSPOT GET PASSED"
else
	echo "SELF TEST SENSPOT GET FAILED $MTEST"
	exit 1
fi
$(pwd)/kill-remote-platform.sh 
ssh ubuntu@$ADDR "rm -f /home/ubuntu/cspot/build/bin/zzzsenspot*"
#echo "sending HUP to $WPID"
#

exit 0


