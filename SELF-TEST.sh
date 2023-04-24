#!/bin/bash
cd build/bin
./woofc-namespace-platform >& namespace.log &
./latency.sh 30
./throughput.sh 1000
kill -HUP %1


