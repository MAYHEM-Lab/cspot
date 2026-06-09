#!/bin/bash
#
KEYCHAIN="$HOME/.cspot/capabilities.yaml"
PRIMARY="woof://cspot-distributions.cs.ucsb.edu/cspot-distributions/release"
PCHECK="1688304179681876176"
BACKUP="woof://169.231.230.76/sharedfs/cspot-distributions"
BCHECK="8096705960766180829"

ARCH=`uname -m`
if ( test "$ARCH" == "aarch64" ) ; then
	TYPE="arm64"
elif ( test "$ARCH" == "x86_64" ) ; then
	TYPE="x86"
else
	echo "unrecognized architecture $ARCH"
	exit 1
fi


if ( ! test -e "$KEYCHAIN" ) ; then
	mkdir -p $HOME/.cspot
	chmod 700 $HOME/.cspot
	echo "namespace:" > $KEYCHAIN
  	echo "    name: $PRIMARY" >> $KEYCHAIN
  	echo "    permissions: 00000001" >> $KEYCHAIN
  	echo "    check: $PCHECK" >> $KEYCHAIN
	echo "namespace:" >> $KEYCHAIN
  	echo "    name: $BACKUP" >> $KEYCHAIN
  	echo "    permissions: 00000001" >> $KEYCHAIN
  	echo "    check: $BCHECK" >> $KEYCHAIN
	chmod 600 $KEYCHAIN
else
	PTEST=`cat $KEYCHAIN | grep 'name: $PRIMARY'`
	if ( test -z "$PTEST" ) ; then
		echo "namespace:" >> $KEYCHAIN
  		echo "    name: $PRIMARY" >> $KEYCHAIN
  		echo "    permissions: 00000001" >> $KEYCHAIN
  		echo "    check: $PCHECK" >> $KEYCHAIN
	fi
	BTEST=`cat $KEYCHAIN | grep 'name: $BACKUP'`
	if ( test -z "$BTEST" ) ; then
		echo "namespace:" >> $KEYCHAIN
  		echo "    name: $BACKUP" >> $KEYCHAIN
  		echo "    permissions: 00000001" >> $KEYCHAIN
  		echo "    check: $BCHECK" >> $KEYCHAIN
	fi
fi

echo "updating from $PRIMARY of latest release"
./senspot-file-recv -f cspot-"$TYPE"-bin.tgz -W $PRIMARY/cspot-"$TYPE"-bin.woof
if ( test $? -eq 0 ) ; then
	tar -xzf cspot-"$TYPE"-bin.tgz
	echo "updated from $PRIMARY of latest release"
else
	echo "$PRIMAY failed, trying $BACKUP"
	./senspot-file-recv -f cspot-"$TYPE"-bin.tgz -W $BACKUP/cspot-"$TYPE"-bin.woof
	if ( test $? -eq 0 ) ; then
		tar -xzf cspot-"$TYPE"-bin.tgz
		echo "updated from $BACKUP of latest release"
	else
		echo "primary $PRIMARY and backup $BACKUP could not be reached"
	fi
fi

