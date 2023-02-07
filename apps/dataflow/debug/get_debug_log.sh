#!/bin/bash

mkdir "logs"

#Only works if there is just a single CSPOTWorker container running:
container_id=$(docker ps -aqf "name=CSPOTWorker")
docker logs "$container_id" > logs/unfiltered_output.log
grep -i "DFHandler:" logs/unfiltered_output.log > logs/filtered_output.log

#Get node stack
(cd ../../../cmake-build-default/bin || exit; ./dfstackdebug) > logs/unfiltered_stack.log
grep -i "DFHandler:" logs/unfiltered_stack.log > logs/filtered_stack.log