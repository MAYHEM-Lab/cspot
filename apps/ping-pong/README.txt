# cspot ping-pong test

There are two versions of the ping pong test -- one that runs in a single
namespace and one that runs across name spaces.  The outpiut of both of these
versions will show up in the standard out and standard errior of the
woofc-namespace-platform for each namespace involved.

The build systemn creates two test namespaces below the source directory in
which the ping-poing code resides.  These instructions describe how to use
these namespaces as a self-test for cspot.

Run make to build the testets and to create the namespaces.   The makefile
assumes that cspot has been successfully built in ../...  It will create the
namespace directories and copy the platform, the container shepherd, and the
handlers into each namespace. 

To run the local handler:

# cd into the namespace directory
cd cspot
# start the namespace platform
./woofc-namespace-platform &> namespace.log
# run the local ping pong test, giving in a size and a name for the woof.  In this example
# the name is ping-pong-test and the size is 10
./ping-pong-start -f ping-pong-test -s 10

# run tail on the namespace log
# If the test is successful then the tail of the namespace log should look like
tail namespace.log 
Forker calling P with Tcount 4
Forker awake, after decrement 3
Parent: count after increment: 4
ping called, counter: 9, seq_no: 10
Forker calling P with Tcount 4
Forker awake, after decrement 3
Parent: count after increment: 4
pong called, counter: 10 seq_no: 11
pong done, counter: 10 seq_no: 11
Reaper: count after increment: 5

To test cross namespace communication, you will need the local IP address that
the namespace container picks up.  The easiest way to pick see what IP address 
the system is using is to start the namespace platform and use ps to pick it up from
the launch line.  For example

ps auxww | grep docker-current

will show the environment variables the docker containers associated with each namespace
are using.  WOOFC_HOST_IP will be set to the IP address that comes back first in a call
to gethostbyname().

In what follows, substitute $IP_ADDR with the IP address used by the containers on the machine.
From the source directory for ping-pong

# start namespace platforms for each namespace
# first name space
cd cspot
./woofc-namespace-platform &> namespace.log &
# second namespace
cd ../cspot-2
./woofc-namespace-platform &> namespace.log &
# run pong start to create pong woof.  Must specify full woof name
export PONG_WPATH=`pwd`
./pong-start -W woof://$IP/$PONG_WPATH/test-pong -s 10
# now cd back to first namespace and run ping-start, specifying two woofs
cd ../cspot
export PING_WPATH=`pwd`
./ping-start -w woof://$IP/$PING_WPATH/test-ping -W woof://$IP/$PONG_WPATH/test-pong -s 100

# look at the tail of both logs.  The tail of the first namespace log for this example should
# look like
tail namespace.log 
ping called, counter: 95, seq_no: 48
Reaper: count after increment: 5
Forker calling P with Tcount 5
Forker awake, after decrement 4
ping called, counter: 97, seq_no: 49
Reaper: count after increment: 5
Forker calling P with Tcount 5
Forker awake, after decrement 4
ping called, counter: 99, seq_no: 50
Reaper: count after increment: 5

# and the tail of the other namespace log
cd ../cspot-2
tail namespace.log 
Forker calling P with Tcount 4
Forker awake, after decrement 3
Parent: count after increment: 4
pong called, counter: 98 seq_no: 50
Forker calling P with Tcount 4
Forker awake, after decrement 3
Parent: count after increment: 4
pong called, counter: 100 seq_no: 51
pong done, counter: 100 seq_no: 51
Reaper: count after increment: 5










