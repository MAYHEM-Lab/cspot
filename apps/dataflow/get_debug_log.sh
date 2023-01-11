#!/bin/bash
#Only works if there is just a single CSPOTWorker container running:
container_id=$(docker ps -aqf "name=CSPOTWorker")
mkdir -p logs
docker logs "$container_id" >logs/unfiltered_output.log
grep -i "DFHandler:" logs/unfiltered_output.log >logs/filtered_output.log
