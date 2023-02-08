#!/bin/bash

mkdir "logs"

#check the time
time1=$(date '+%H_%M_%S')
#crate output directory
outDirName="logs_$time1"
mkdir logs/"$outDirName"
#goto output directory
pushd "logs/$outDirName" || exit

#Only works if there is just a single CSPOTWorker container running:
container_id=$(docker ps -aqf "name=CSPOTWorker")
docker logs "$container_id" 2> unfiltered_container_output.log > unfiltered_output.log
grep -i "DFHandler:" unfiltered_output.log > filtered_output.log

#Get node stack
(cd ../../../../../cmake-build-default/bin || exit; ./dfnodestackdebug) > unfiltered_node_stack.log
grep -i "DFHandler:" unfiltered_node_stack.log > filtered_node_stack.log

#Get operand stack
(cd ../../../../../cmake-build-default/bin || exit; ./dfoperandstackdebug) > unfiltered_operand_stack.log
grep -i "DFHandler:" unfiltered_operand_stack.log > filtered_operand_stack.log

#restore original directory
popd > /dev/null || exit
#where am i?
echo "Logs saved in the output directory:  $(pwd)/$outDirName"
