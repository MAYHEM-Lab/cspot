#!/bin/bash -f
#
source ../../../../../cicd-config.sh
#source ~/actions-runner/cicd-config.sh
ADDR=$X86HELPER
$(pwd)/start-remote-platform.sh
#do regular file tests first
ssh ubuntu@$ADDR "cd /home/ubuntu/cspot/build/bin && ./senspot-file-init -W zzzfile -M 4 -s 1024"
echo "zzzfile created"

ssh ubuntu@$ADDR "cd /home/ubuntu/cspot/build/bin && ./senspot-file-init -W zzzfilebig -M 64 -s 10"
echo "zzzfilebig created"
# create test file
rm -f ./zzzfff
cp ../../cicd-tests/zzzfff .

FTEST=`./senspot-file-send -f ./zzzfff -W woof://$ADDR/home/ubuntu/cspot/build/bin/zzzfile -V | grep "megabytes"`
if ( test -z "$FTEST" ) ; then
	echo "senspot-file-send failed"
	rm -f ./zzzfff
	ssh ubuntu@$ADDR "rm -f /home/ubuntu/cspot/build/bin/zzzfile*"
	$(pwd)/kill-remote-platform.sh 
	exit 1
fi

RTEST=`./senspot-file-recv -f ./zzzggg -W woof://$ADDR/home/ubuntu/cspot/build/bin/zzzfile -V | grep "megabytes"`

if ( test -z "$RTEST" ) ; then
	echo "senspot-file-recv failed"
	rm -f ./zzzfff ./zzzggg
	ssh ubuntu@$ADDR "rm -f /home/ubuntu/cspot/build/bin/zzzfile*"
	$(pwd)/kill-remote-platform.sh 
	exit 1
fi

DTEST=`diff ./zzzfff ./zzzggg`

if ( ! test -z "$DTEST" ) ; then
	echo "file diff failed"
	rm -f ./zzzfff ./zzzggg
	ssh ubuntu@$ADDR "rm -f /home/ubuntu/cspot/build/bin/zzzfile*"
	$(pwd)/kill-remote-platform.sh 
	exit 1
fi

FTEST=`./senspot-file-send -f ./zzzfff -W woof://$ADDR/home/ubuntu/cspot/build/bin/zzzfilebig -V | grep "megabytes"`
if ( test -z "$FTEST" ) ; then
	echo "senspot-file-send big failed"
	rm -f ./zzzfff
	ssh ubuntu@$ADDR "rm -f /home/ubuntu/cspot/build/bin/zzzfile*"
	$(pwd)/kill-remote-platform.sh 
	exit 1
fi

RTEST=`./senspot-file-recv -f ./zzzggg -W woof://$ADDR/home/ubuntu/cspot/build/bin/zzzfilebig -V | grep "megabytes"`

if ( test -z "$RTEST" ) ; then
	echo "senspot-file-recv big failed"
	rm -f ./zzzfff ./zzzggg
	ssh ubuntu@$ADDR "rm -f /home/ubuntu/cspot/build/bin/zzzfile*"
	$(pwd)/kill-remote-platform.sh 
	exit 1
fi

DTEST=`diff ./zzzfff ./zzzggg`

if ( ! test -z "$DTEST" ) ; then
	echo "file diff big failed"
	rm -f ./zzzfff ./zzzggg
	ssh ubuntu@$ADDR "rm -f /home/ubuntu/cspot/build/bin/zzzfile*"
	$(pwd)/kill-remote-platform.sh 
	exit 1
fi

$(pwd)/kill-remote-platform.sh 
ssh ubuntu@$ADDR "rm -f /home/ubuntu/cspot/build/bin/zzzfile*"
#echo "sending HUP to $WPID"
#
#

echo "SENSPOT-FILE TEST PASSED"

exit 0


