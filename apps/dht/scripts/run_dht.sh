mkdir -p dht_log

echo 'Starting daemons'
ssh dht1 'cd ~/cspot1/apps/dht/cspot/; ./woofc-namespace-platform' &> dht_log/dht1_log &
ssh dht2 'cd ~/cspot2/apps/dht/cspot/; ./woofc-namespace-platform' &> dht_log/dht2_log &
ssh dht3 'cd ~/cspot3/apps/dht/cspot/; ./woofc-namespace-platform' &> dht_log/dht3_log &
ssh dht4 'cd ~/cspot4/apps/dht/cspot/; ./woofc-namespace-platform' &> dht_log/dht4_log &
ssh dht5 'cd ~/cspot5/apps/dht/cspot/; ./woofc-namespace-platform' &> dht_log/dht5_log &
ssh dht6 'cd ~/cspot6/apps/dht/cspot/; ./woofc-namespace-platform' &> dht_log/dht6_log &
ssh dht7 'cd ~/cspot7/apps/dht/cspot/; ./woofc-namespace-platform' &> dht_log/dht7_log &
ssh dht8 'cd ~/cspot8/apps/dht/cspot/; ./woofc-namespace-platform' &> dht_log/dht8_log &

(tail -f dht_log/dht1_log & ) | grep -q "woofc-container: started message server"
(tail -f dht_log/dht2_log & ) | grep -q "woofc-container: started message server"
(tail -f dht_log/dht3_log & ) | grep -q "woofc-container: started message server"
(tail -f dht_log/dht4_log & ) | grep -q "woofc-container: started message server"
(tail -f dht_log/dht5_log & ) | grep -q "woofc-container: started message server"
(tail -f dht_log/dht6_log & ) | grep -q "woofc-container: started message server"
(tail -f dht_log/dht7_log & ) | grep -q "woofc-container: started message server"
(tail -f dht_log/dht8_log & ) | grep -q "woofc-container: started message server"

echo 'Starting DHT cluster'
ssh dht1 'cd ~/cspot1/apps/dht/cspot/; ./create' > dht_log/dht1_create
ssh dht1 'cd ~/cspot1/apps/dht/cspot/; ./dht_daemon' &> dht_log/dht1_daemon_log &
echo 'DHT1 started'
ssh dht2 'cd ~/cspot2/apps/dht/cspot/; ./join -w woof://10.1.4.138:53483/home/centos/cspot1/apps/dht/cspot' > dht_log/dht2_join
ssh dht2 'cd ~/cspot2/apps/dht/cspot/; ./dht_daemon' &> dht_log/dht2_daemon_log &
echo 'DHT2 started'
ssh dht3 'cd ~/cspot3/apps/dht/cspot/; ./join -w woof://10.1.4.138:53483/home/centos/cspot1/apps/dht/cspot' > dht_log/dht3_join
ssh dht3 'cd ~/cspot3/apps/dht/cspot/; ./dht_daemon' &> dht_log/dht3_daemon_log &
echo 'DHT3 started'
ssh dht4 'cd ~/cspot4/apps/dht/cspot/; ./join -w woof://10.1.4.138:53483/home/centos/cspot1/apps/dht/cspot' > dht_log/dht4_join
ssh dht4 'cd ~/cspot4/apps/dht/cspot/; ./dht_daemon' &> dht_log/dht4_daemon_log &
echo 'DHT4 started'
ssh dht5 'cd ~/cspot5/apps/dht/cspot/; ./join -w woof://10.1.4.138:53483/home/centos/cspot1/apps/dht/cspot' > dht_log/dht5_join
ssh dht5 'cd ~/cspot5/apps/dht/cspot/; ./dht_daemon' &> dht_log/dht5_daemon_log &
echo 'DHT5 started'
ssh dht6 'cd ~/cspot6/apps/dht/cspot/; ./join -w woof://10.1.4.138:53483/home/centos/cspot1/apps/dht/cspot' > dht_log/dht6_join
ssh dht6 'cd ~/cspot6/apps/dht/cspot/; ./dht_daemon' &> dht_log/dht6_daemon_log &
echo 'DHT6 started'
ssh dht7 'cd ~/cspot7/apps/dht/cspot/; ./join -w woof://10.1.4.138:53483/home/centos/cspot1/apps/dht/cspot' > dht_log/dht7_join
ssh dht7 'cd ~/cspot7/apps/dht/cspot/; ./dht_daemon' &> dht_log/dht7_daemon_log &
echo 'DHT7 started'
ssh dht8 'cd ~/cspot8/apps/dht/cspot/; ./join -w woof://10.1.4.138:53483/home/centos/cspot1/apps/dht/cspot' > dht_log/dht8_join
ssh dht8 'cd ~/cspot8/apps/dht/cspot/; ./dht_daemon' &> dht_log/dht8_daemon_log &
echo 'DHT8 started'