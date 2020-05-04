ssh dht1 'sudo pkill -9 woof; sudo pkill -9 dht_daemon' &> /dev/null &
ssh dht2 'sudo pkill -9 woof; sudo pkill -9 dht_daemon' &> /dev/null &
ssh dht3 'sudo pkill -9 woof; sudo pkill -9 dht_daemon' &> /dev/null &
ssh dht4 'sudo pkill -9 woof; sudo pkill -9 dht_daemon' &> /dev/null &
ssh dht5 'sudo pkill -9 woof; sudo pkill -9 dht_daemon' &> /dev/null &
ssh dht6 'sudo pkill -9 woof; sudo pkill -9 dht_daemon' &> /dev/null &
ssh dht7 'sudo pkill -9 woof; sudo pkill -9 dht_daemon' &> /dev/null &
ssh dht8 'sudo pkill -9 woof; sudo pkill -9 dht_daemon' &> /dev/null &
ssh dht9 'sudo pkill -9 woof; sudo pkill -9 dht_daemon' &> /dev/null &

while [[ $(ssh dht1 'docker ps -aq') != '' ]]
do
    ssh dht1 'docker rm -f '$(ssh dht1 'docker ps -aq') &> /dev/null
done

while [[ $(ssh dht2 'docker ps -aq') != '' ]]
do
    ssh dht2 'docker rm -f '$(ssh dht2 'docker ps -aq') &> /dev/null
    sleep 0.1
done

while [[ $(ssh dht3 'docker ps -aq') != '' ]]
do
    ssh dht3 'docker rm -f '$(ssh dht3 'docker ps -aq') &> /dev/null
    sleep 0.1
done

while [[ $(ssh dht4 'docker ps -aq') != '' ]]
do
    ssh dht4 'docker rm -f '$(ssh dht4 'docker ps -aq') &> /dev/null
    sleep 0.1
done

while [[ $(ssh dht5 'docker ps -aq') != '' ]]
do
    ssh dht5 'docker rm -f '$(ssh dht5 'docker ps -aq') &> /dev/null
    sleep 0.1
done

while [[ $(ssh dht6 'docker ps -aq') != '' ]]
do
    ssh dht6 'docker rm -f '$(ssh dht6 'docker ps -aq') &> /dev/null
    sleep 0.1
done

while [[ $(ssh dht7 'docker ps -aq') != '' ]]
do
    ssh dht7 'docker rm -f '$(ssh dht7 'docker ps -aq') &> /dev/null
    sleep 0.1
done

while [[ $(ssh dht8 'docker ps -aq') != '' ]]
do
    ssh dht8 'docker rm -f '$(ssh dht8 'docker ps -aq') &> /dev/null
    sleep 0.1
done