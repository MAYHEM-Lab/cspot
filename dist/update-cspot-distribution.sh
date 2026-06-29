#!/bin/bash
# installer

SUBDIR=$1

if ( test -z "$SUBDIR" ) ; then
	SUBDIR="release"
fi

if ( test "$SUBDIR" != "release" ) ; then
	if ( test "$SUBDIR" != "daily" ) ; then
		echo "must specify either release or daily"
		exit 1
	fi
fi

CMD=$2

if [[ "$CMD" != "" && "$CMD" != "lib" && "$CMD" != "bin" && "$CMD" != "all" ]] ; then
	echo "supported updates are 'all', 'lib' or 'bin'"
	exit 1
fi

KEYCHAIN="$HOME/.cspot/capabilities.yaml"

PRIMARY="woof://cspot-distributions.cs.ucsb.edu/cspot-distributions/$SUBDIR"
BACKUP="woof://169.231.229.94/cspot-distributions/$SUBDIR"
BACKUP2="woof://169.231.230.76/sharedfs/cspot-distributions"
if ( test "$SUBDIR" == "release" ) ; then
	PCHECK="1688304179681876176"
	BCHECK="423494180182850117"
	BCHECK2="8096705960766180829"
else
	PCHECK="10022175549340067307"
	BCHECK="8476964376741850441"
fi


ARCH=`uname -m`
if ( test "$ARCH" == "aarch64" ) ; then
	TYPE="arm64"
elif ( test "$ARCH" == "x86_64" ) ; then
	TYPE="x86"
elif ( test "$ARCH" == "arm64" ) ; then
	TYPE="arm64-apple"
else
	echo "unrecognized architecture $ARCH"
	exit 1
fi

HERE=`pwd`


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
	echo "namespace:" >> $KEYCHAIN
  	echo "    name: $BACKUP2" >> $KEYCHAIN
  	echo "    permissions: 00000001" >> $KEYCHAIN
  	echo "    check: $BCHECK2" >> $KEYCHAIN
	chmod 600 $KEYCHAIN
else
	PTEST=`cat $KEYCHAIN | grep "name: $PRIMARY"`
	if ( test -z "$PTEST" ) ; then
                echo "namespace:" >> $KEYCHAIN
                echo "    name: $PRIMARY" >> $KEYCHAIN
                echo "    permissions: 00000001" >> $KEYCHAIN
                echo "    check: $PCHECK" >> $KEYCHAIN
	fi
        BTEST=`cat $KEYCHAIN | grep "name: $BACKUP"`
        if ( test -z "$BTEST" ) ; then
                echo "namespace:" >> $KEYCHAIN
                echo "    name: $BACKUP" >> $KEYCHAIN
                echo "    permissions: 00000001" >> $KEYCHAIN
                echo "    check: $BCHECK" >> $KEYCHAIN
        fi
	if ( test "$SUBDIR" == "release" ) ; then
        	BTEST2=`cat $KEYCHAIN | grep "name: $BACKUP2"`
        	if ( test -z "$BTEST2" ) ; then
                	echo "namespace:" >> $KEYCHAIN
                	echo "    name: $BACKUP2" >> $KEYCHAIN
                	echo "    permissions: 00000001" >> $KEYCHAIN
                	echo "    check: $BCHECK2" >> $KEYCHAIN
        	fi
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

BINOK=0
if [[ "$CMD" == "bin" || "$CMD" == "all" || "$CMD" == "" ]] ; then
	echo "contacting repo at $PRIMARY for bin software"
	$HERE/senspot-file-recv.$TYPE -f $HERE/cspot-"$TYPE"-bin.tgz -W $PRIMARY/cspot-"$TYPE"-bin.woof
	if ( test $? -eq 0 ) ; then
		$HERE/senspot-file-recv.$TYPE -f $HERE/cspot-"$TYPE"-bin.sha256 -W $PRIMARY/cspot-"$TYPE"-bin-sha256.woof
		if ( test $? -eq 0 ) ; then
			BINOK=1
			LOCATION=$PRIMARY
		fi
	fi
	if ( test $BINOK -eq 0 ) ; then
		echo "$PRIMARY failed to respond"
		echo "contacting repo at $BACKUP for bin software"
		$HERE/senspot-file-recv.$TYPE -f $HERE/cspot-"$TYPE"-bin.tgz -W $BACKUP/cspot-"$TYPE"-bin.woof
		if ( test $? -eq 0 ) ; then
			$HERE/senspot-file-recv.$TYPE -f $HERE/cspot-"$TYPE"-bin.sha256 -W $BACKUP/cspot-"$TYPE"-bin-sha256.woof
			BINOK=1
			LOCATION=$BACKUP
		fi
	fi
	if ( test $BINOK -eq 0 ) ; then
		if ( test "$SUBDIR" == "daily" ) ; then
			echo "could not fetch daily bin from $PRIMARY or $BACKUP"
			exit 1
		fi
	fi
	if [[ "$SUBDIR" == "release" && $BINOK -eq 0 ]] ; then 
		echo "$BACKUP failed to respond"
		echo "contacting repo at $BACKUP2 for bin software"
		$HERE/senspot-file-recv.$TYPE -f $HERE/cspot-"$TYPE"-bin.tgz -W $BACKUP2/cspot-"$TYPE"-bin.woof
		if ( test $? -eq 0 ) ; then
			$HERE/senspot-file-recv.$TYPE -f $HERE/cspot-"$TYPE"-bin.sha256 -W $BACKUP2/cspot-"$TYPE"-bin-sha256.woof
			BINOK=1
			LOCATION=$BACKUP2
		fi
		if ( test $BINOK -eq 0 ) ; then
			echo "could not fetch daily bin from $PRIMARY or $BACKUP or $BACKUP2"
		fi
	fi
fi

LIBOK=0
if [[ "$CMD" == "lib" || "$CMD" == "all" ]] ; then
	echo "contacting repo at $PRIMARY for lib software"
	$HERE/senspot-file-recv.$TYPE -f $HERE/cspot-"$TYPE"-lib.tgz -W $PRIMARY/cspot-"$TYPE"-lib.woof
	if ( test $? -eq 0 ) ; then
		$HERE/senspot-file-recv.$TYPE -f $HERE/cspot-"$TYPE"-lib.sha256 -W $PRIMARY/cspot-"$TYPE"-lib-sha256.woof
		if ( test $? -eq 0 ) ; then
			LIBOK=1
			LIBLOCATION=$PRIMARY
		fi
	fi
	if ( test $LIBOK -eq 0 ) ; then
		echo "$PRIMARY failed to respond"
		echo "contacting repo at $BACKUP for lib software"
		$HERE/senspot-file-recv.$TYPE -f $HERE/cspot-"$TYPE"-lib.tgz -W $BACKUP/cspot-"$TYPE"-lib.woof
		if ( test $? -eq 0 ) ; then
			$HERE/senspot-file-recv.$TYPE -f $HERE/cspot-"$TYPE"-lib.sha256 -W $BACKUP/cspot-"$TYPE"-lib-sha256.woof
			LIBOK=1
			LIBLOCATION=$BACKUP
		fi
	fi
	if ( test $LIBOK -eq 0 ) ; then
		if ( test "$SUBDIR" == "daily" ) ; then
			echo "could not fetch daily lib from $PRIMARY or $BACKUP"
			exit 1
		fi
	fi
	if [[ "$SUBDIR" == "release" && $LIBOK -eq 0 ]] ; then
		echo "$BACKUP failed to respond"
		echo "contacting repo at $BACKUP2 for lib software"
		$HERE/senspot-file-recv.$TYPE -f $HERE/cspot-"$TYPE"-lib.tgz -W $BACKUP2/cspot-"$TYPE"-lib.woof
		if ( test $? -eq 0 ) ; then
			$HERE/senspot-file-recv.$TYPE -f $HERE/cspot-"$TYPE"-lib.sha256 -W $BACKUP2/cspot-"$TYPE"-lib-sha256.woof
			LIBOK=1
			LIBLOCATION=$BACKUP2
		fi
		if ( test $LIBOK -eq 0 ) ; then
			echo "could not fetch daily lib from $PRIMARY or $BACKUP or $BACKUP2"
		fi
	fi
fi

if [[ "$CMD" == "bin" || "$CMD" == "all" || "$CMD" == "" ]] ; then
	SHKEY=`tail -n 1 $HERE/cspot-"$TYPE"-bin.sha256 | awk '{print $1}'`
	if ( test "$TYPE" == "arm64-apple" ) ; then
		LKEY=`shasum -a 256 $HERE/cspot-"$TYPE"-bin.tgz | awk '{print $1}'`
	else
		LKEY=`sha256sum $HERE/cspot-"$TYPE"-bin.tgz | awk '{print $1}'`
	fi

	if ( test -z "LKEY" ) ; then
		echo "local sha256 could not be computed for bin"
		echo "bin software not installed"
		exit 1
	fi

	if ( test -z "SHKEY" ) ; then
		echo "remote sha256 could not be computed for bin"
		echo "bin software not installed"
		exit 1
	fi

	if ( test "$SHKEY" == "$LKEY" ) ; then
		echo "updating cspot bin $SUBDIR from $LOCATION at " `/bin/date`
		tar -xzf $HERE/cspot-"$TYPE"-bin.tgz
	else
		echo "local hash: " $LKEY "does not match remote hash" $SHKEY "for bin"
		echo "bin software not installed"
		exit 1
	fi
fi

if [[ "$CMD" == "lib" || "$CMD" == "all" ]] ; then
	SHKEY=`tail -n 1 $HERE/cspot-"$TYPE"-lib.sha256 | awk '{print $1}'`
	if ( test "$TYPE" == "arm64-apple" ) ; then
		LKEY=`shasum -a 256 $HERE/cspot-"$TYPE"-lib.tgz | awk '{print $1}'`
	else
		LKEY=`sha256sum $HERE/cspot-"$TYPE"-lib.tgz | awk '{print $1}'`
	fi

	if ( test -z "LKEY" ) ; then
		echo "local sha256 could not be computed for lib"
		echo "lib software not installed"
		exit 1
	fi

	if ( test -z "SHKEY" ) ; then
		echo "remote sha256 could not be computed for lib"
		echo "lib software not installed"
		exit 1
	fi

	if ( test "$SHKEY" == "$LKEY" ) ; then
		echo "updating cspot lib $SUBDIR from $LIBLOCATION at " `/bin/date`
		tar -xzf $HERE/cspot-"$TYPE"-lib.tgz
	else
		echo "local hash: " $LKEY "does not match remote hash" $SHKEY "for lib"
		echo "lib software not installed"
		exit 1
	fi
fi
