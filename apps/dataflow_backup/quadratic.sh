#!/bin/bash


#usage: quadratic.sh a b c

A="$1"
if ( test -z "$A" ) ; then
	echo "usage: quadratic.sh a b c"
	exit 1
fi

B="$2"
if ( test -z "$B" ) ; then
	echo "usage: quadratic.sh a b c"
	exit 1
fi

C="$3"
if ( test -z "$C" ) ; then
	echo "usage: quadratic.sh a b c"
	exit 1
fi

# from dataflow/df.h

ADD=1
SUB=2
MUL=3
DIV=4
SQR=5

./dfinit -W test -s 10000

#dfaddnode -W woofname
#	-i node-id
#	-o node-opcode
#	-d dest-node-id-for-result
#	-1 <dest operand must be first for non-commute dest>
./dfaddnode -W test -o $DIV -i 1 -d 0 # -b + sqr(b^2 - 4ac) / 2a -> result
./dfaddnode -W test -o $MUL -i 2 -d 1 # 2a -> node 1
./dfaddnode -W test -o $MUL -i 3 -d 4 # -1 * b -> node 4
./dfaddnode -W test -o $ADD -i 4 -d 1 -1 # (-1 * b) + sqr(b^2 - 4ac) -> node 1, order 1
./dfaddnode -W test -o $SQR -i 5 -d 4 -1 # sqr(b^2 - 4ac) -> node 4
./dfaddnode -W test -o $SUB -i 6 -d 5 # b^2 - 4ac -> node 5
./dfaddnode -W test -o $MUL -i 7 -d 6 -1 # b^2 -> node 6, order 1
./dfaddnode -W test -o $MUL -i 8 -d 6 # 4ac -> node 6
./dfaddnode -W test -o $MUL -i 9 -d 8 # ac -> node 8

#dfaddoperand -W woofname
#	-d dest-node-id-for-result
#	-V value
#	-1 <must be first operand to dest in non-commute case>
./dfaddoperand -W test -d 7 -V $B
./dfaddoperand -W test -d 7 -V $B
./dfaddoperand -W test -d 8 -V 4.0
./dfaddoperand -W test -d 9 -V $A
./dfaddoperand -W test -d 9 -V $C
./dfaddoperand -W test -d 3 -V -1.0
./dfaddoperand -W test -d 3 -V $B
./dfaddoperand -W test -d 2 -V 2.0
./dfaddoperand -W test -d 2 -V $A
./dfaddoperand -W test -d 5 -V 1.0 # ignored because this is SQR and need two op
