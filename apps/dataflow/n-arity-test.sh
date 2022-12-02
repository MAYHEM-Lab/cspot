#!/bin/bash
# from dataflow/df.h

ADD=1
SUB=2
MUL=3
DIV=4
SQR=5

./dfinit -W test -s 10000

#dfaddnode -W woofname
#   -i node-id
#   -o node-opcode
#   -d dest-node-id-for-result
#   -p dest-port-id-for-result
#   -n node-arity

./dfaddnode -W test -o $MUL -i 1 -d 0 -p 0 -n 2   
./dfaddnode -W test -o $ADD -i 2 -d 1 -p 0 -n 4
./dfaddnode -W test -o $ADD -i 3 -d 1 -p 1 -n 3

#dfaddoperand -W woofname
#   -d dest-node-id-for-result
#   -p dest-port-id-for-result
#   -V value
./dfaddoperand -W test -d 2 -p 0 -V 1
./dfaddoperand -W test -d 2 -p 1 -V 2
./dfaddoperand -W test -d 2 -p 2 -V 3
./dfaddoperand -W test -d 2 -p 3 -V 4

./dfaddoperand -W test -d 3 -p 0 -V 4
./dfaddoperand -W test -d 3 -p 1 -V 5
./dfaddoperand -W test -d 3 -p 2 -V 6




