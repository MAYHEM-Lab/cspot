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
rm -f ./zzzfff ./zzzxxx
cp ../../cicd-tests/zzzfff .
cp ../../cicd-tests/zzzxxx .

FTEST=`./senspot-file-send -f ./zzzfff -W woof://$ADDR/home/ubuntu/cspot/build/bin/zzzfile -V | grep "megabytes"`
if ( test -z "$FTEST" ) ; then
	echo "senspot-file-send failed"
	rm -f ./zzzfff ./zzzxxx
	ssh ubuntu@$ADDR "rm -f /home/ubuntu/cspot/build/bin/zzzfile*"
	$(pwd)/kill-remote-platform.sh 
	exit 1
fi

RTEST=`./senspot-file-recv -f ./zzzggg -W woof://$ADDR/home/ubuntu/cspot/build/bin/zzzfile -V | grep "megabytes"`

if ( test -z "$RTEST" ) ; then
	echo "senspot-file-recv failed"
	rm -f ./zzzfff ./zzzggg ./zzzxxx
	ssh ubuntu@$ADDR "rm -f /home/ubuntu/cspot/build/bin/zzzfile*"
	$(pwd)/kill-remote-platform.sh 
	exit 1
fi

DTEST=`diff ./zzzfff ./zzzggg`

if ( ! test -z "$DTEST" ) ; then
	echo "file diff failed"
	rm -f ./zzzfff ./zzzggg ./zzzxxx
	ssh ubuntu@$ADDR "rm -f /home/ubuntu/cspot/build/bin/zzzfile*"
	$(pwd)/kill-remote-platform.sh 
	exit 1
fi

FTEST=`./senspot-file-send -f ./zzzfff -W woof://$ADDR/home/ubuntu/cspot/build/bin/zzzfilebig -V | grep "megabytes"`
if ( test -z "$FTEST" ) ; then
	echo "senspot-file-send big failed"
	rm -f ./zzzfff ./zzzggg ./zzzxxx
	ssh ubuntu@$ADDR "rm -f /home/ubuntu/cspot/build/bin/zzzfile*"
	$(pwd)/kill-remote-platform.sh 
	exit 1
fi

RTEST=`./senspot-file-recv -f ./zzzggg -W woof://$ADDR/home/ubuntu/cspot/build/bin/zzzfilebig -V | grep "megabytes"`

if ( test -z "$RTEST" ) ; then
	echo "senspot-file-recv big failed"
	rm -f ./zzzfff ./zzzggg ./zzzxxx
	ssh ubuntu@$ADDR "rm -f /home/ubuntu/cspot/build/bin/zzzfile*"
	$(pwd)/kill-remote-platform.sh 
	exit 1
fi

DTEST=`diff ./zzzfff ./zzzggg`

if ( ! test -z "$DTEST" ) ; then
	echo "file diff big failed"
	rm -f ./zzzfff ./zzzggg ./zzzxxx
	ssh ubuntu@$ADDR "rm -f /home/ubuntu/cspot/build/bin/zzzfile*"
	$(pwd)/kill-remote-platform.sh 
	exit 1
fi

# try multi-write and versioning
./senspot-file-send -f ./zzzfff -W woof://$ADDR/home/ubuntu/cspot/build/bin/zzzfile &
./senspot-file-send -f ./zzzfff -W woof://$ADDR/home/ubuntu/cspot/build/bin/zzzfile &

sleep 1
FTEST=`./senspot-file-send -f ./zzzxxx -W woof://$ADDR/home/ubuntu/cspot/build/bin/zzzfilebig -V | grep "megabytes"`

if ( test -z "$FTEST" ) ; then
	echo "second file send failed"
	rm -f ./zzzfff ./zzzggg ./zzzxxx
	ssh ubuntu@$ADDR "rm -f /home/ubuntu/cspot/build/bin/zzzfile*"
	$(pwd)/kill-remote-platform.sh 
	exit 1
fi

VER=`./senspot-file-recv -W woof://$ADDR/home/ubuntu/cspot/build/bin/zzzfile -L | head -n 2 | tail -n 1 | awk '{print $2}'`
MAJ=`echo $VER | awk -F ':' '{print $1}'`
MIN=`echo $VER | awk -F ':' '{print $2}'`

echo "fetching version $VER"
#./senspot-file-recv -W woof://$ADDR/home/ubuntu/cspot/build/bin/zzzfile -L
RTEST=`./senspot-file-recv -f ./zzzyyy -W woof://$ADDR/home/ubuntu/cspot/build/bin/zzzfile -v $MAJ -m $MIN -V | grep "megabytes"`
if ( test -z "$RTEST" ) ; then
	echo "coupld not fetch version $VER"
	rm -f ./zzzfff ./zzzggg ./zzzxxx ./zzzyyy
	ssh ubuntu@$ADDR "rm -f /home/ubuntu/cspot/build/bin/zzzfile*"
	$(pwd)/kill-remote-platform.sh 
	exit 1
fi


DTEST1=`diff ./zzzfff ./zzzyyy`
DTEST2=`diff ./zzzxxx ./zzzyyy`
if ( ! test -z "$DTEST1" ) ; then
	if ( ! test -z "$DTEST2" ) ; then
		echo "diff fails for multi version"
		rm -f ./zzzfff ./zzzggg ./zzzxxx ./zzzyyy
		ssh ubuntu@$ADDR "rm -f /home/ubuntu/cspot/build/bin/zzzfile*"
		$(pwd)/kill-remote-platform.sh 
		exit 1
	fi
fi

$(pwd)/kill-remote-platform.sh 
ssh ubuntu@$ADDR "rm -f /home/ubuntu/cspot/build/bin/zzzfile*"
rm -f ./zzzfff ./zzzggg ./zzzxxx ./zzzyyy
#echo "sending HUP to $WPID"
#
#

echo "SENSPOT-FILE TEST PASSED"

exit 0


