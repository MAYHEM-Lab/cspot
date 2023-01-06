#!/bin/bash
#Only works if there is just a single CSPOTWorker container running:
container_id=$(docker ps -aqf "name=CSPOTWorker")
docker logs "$container_id" > unfiltered_output.log
grep -i "DFHandler:" unfiltered_output.log > filtered_output.log