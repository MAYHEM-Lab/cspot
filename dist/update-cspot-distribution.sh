#!/bin/bash
# installer
KEYCHAIN="$HOME/.cspot/capabilities.yaml"

ARCH=`uname -m`
if ( test "$ARCH" == "aarch64" ) ; then
	TYPE="arm64"
elif ( test "$ARCH" == "x86_64" ) ; then
	TYPE="x86"
else
	echo "unrecognized architecture $ARCH"
	exit 1
fi

HERE=`pwd`


if ( ! test -e "$KEYCHAIN" ) ; then
	mkdir -p $HOME/.cspot
	chmod 700 $HOME/.cspot
	echo "namespace:" > $KEYCHAIN
  	echo "    name: woof://169.231.230.76/sharedfs/cspot-distributions" >> $KEYCHAIN
  	echo "    permissions: 00000001" >> $KEYCHAIN
  	echo "    check: 8096705960766180829" >> $KEYCHAIN
	chmod 600 $KEYCHAIN
else
	CTEST=`cat $KEYCHAIN | grep 'name: woof://169.231.230.76/sharedfs/cspot-distributions'`
	if ( test -z "$CTEST" ) ; then
		echo "namespace:" >> $KEYCHAIN
  		echo "    name: woof://169.231.230.76/sharedfs/cspot-distributions" >> $KEYCHAIN
  		echo "    permissions: 00000001" >> $KEYCHAIN
  		echo "    check: 8096705960766180829" >> $KEYCHAIN
	fi
fi

# get the base recv binary if necessary
if ( ! test -e "$HERE/senspot-file-recv.$TYPE" ) ; then
	curl -fsSL https://raw.githubusercontent.com/MAYHEM-Lab/cspot/caplets/dist/senspot-file-recv.$TYPE -o $HERE/senspot-file-recv.$TYPE
	chmod 700 $HERE/senspot-file-recv.$TYPE
fi

if ( ! test -e "$HERE/update-cspot-distribution.sh" ) ; then
	curl -fsSL https://raw.githubusercontent.com/MAYHEM-Lab/cspot/caplets/dist/update-cspot-distribution.sh -o $HERE/update-cspot-distribution.sh
	chmod 700 $HERE/update-cspot-distribution.sh
fi

$HERE/senspot-file-recv.$TYPE -f $HERE/cspot-"$TYPE"-bin.tgz -W woof://169.231.230.76/sharedfs/cspot-distributions/cspot-"$TYPE"-bin.woof
$HERE/senspot-file-recv.$TYPE -f $HERE/cspot-"$TYPE"-bin.sha256 -W woof://169.231.230.76/sharedfs/cspot-distributions/cspot-"$TYPE"-bin-sha256.woof
SHKEY=`tail -n 1 $HERE/cspot-"$TYPE"-bin.sha256 | awk '{print $1}'`
LKEY=`sha256sum $HERE/cspot-"$TYPE"-bin.tgz | awk '{print $1}'`

if ( test -z "LKEY" ) ; then
	echo "local sha256 could not be computed"
	echo "software not installed"
	exit 1
fi

if ( test -z "SHKEY" ) ; then
	echo "remote sha256 could not be computed"
	echo "software not installed"
	exit 1
fi

if ( test "$SHKEY" == "$LKEY" ) ; then
	echo "updating cspot at " `/bin/date`
	tar -xzf $HERE/cspot-"$TYPE"-bin.tgz
else
	echo "local hash: " $LKEY "does not match remote hash" $SHKEY
	echo "software not installed"
	exit 1
fi
