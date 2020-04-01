ssh dht1 'sudo pkill -9 woof' &
ssh dht2 'sudo pkill -9 woof' &
ssh dht3 'sudo pkill -9 woof' &

sleep 2

containers=$(ssh dht1 'docker ps -aq')
while [[ $containers != '' ]]
do
    ssh dht1 'docker rm '$containers
    sleep 0.1
    containers=$(ssh dht1 'docker ps -aq')
done

containers=$(ssh dht2 'docker ps -aq')
while [[ $containers != '' ]]
do
    ssh dht2 'docker rm '$containers
    sleep 0.1
    containers=$(ssh dht2 'docker ps -aq')
done

containers=$(ssh dht3 'docker ps -aq')
while [[ $containers != '' ]]
do
    ssh dht3 'docker rm '$containers
    sleep 0.1
    containers=$(ssh dht3 'docker ps -aq')
done

# containers=$(ssh dht4 'docker ps -aq')
# while [[ $containers != '' ]]
# do
#     ssh dht4 'docker rm '$containers
#     sleep 0.1
#     containers=$(ssh dht4 'docker ps -aq')
# done

# containers=$(ssh dht5 'docker ps -aq')
# while [[ $containers != '' ]]
# do
#     ssh dht5 'docker rm '$containers
#     sleep 0.1
#     containers=$(ssh dht5 'docker ps -aq')
# done

# containers=$(ssh dht6 'docker ps -aq')
# while [[ $containers != '' ]]
# do
#     ssh dht6 'docker rm '$containers
#     sleep 0.1
#     containers=$(ssh dht6 'docker ps -aq')
# done

# containers=$(ssh dht7 'docker ps -aq')
# while [[ $containers != '' ]]
# do
#     ssh dht7 'docker rm '$containers
#     sleep 0.1
#     containers=$(ssh dht7 'docker ps -aq')
# done

# containers=$(ssh dht8 'docker ps -aq')
# while [[ $containers != '' ]]
# do
#     ssh dht8 'docker rm '$containers
#     sleep 0.1
#     containers=$(ssh dht8 'docker ps -aq')
# done