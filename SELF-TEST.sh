#!/bin/bash
#cd build/bin
cp ../../apps/self-test/latency.sh .
cp ../../apps/self-test/throughput.sh .
./woofc-namespace-platform >& namespace.log &
./latency.sh 5 
./throughput.sh 100 
kill -HUP %1


