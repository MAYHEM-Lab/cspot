mkdir -p raft_log

echo 'Starting daemons'
ssh dht1 'cd ~/cspot1/apps/raft/cspot/; ./woofc-namespace-platform' &> raft_log/dht1_log &
ssh dht2 'cd ~/cspot2/apps/raft/cspot/; ./woofc-namespace-platform' &> raft_log/dht2_log &
ssh dht3 'cd ~/cspot3/apps/raft/cspot/; ./woofc-namespace-platform' &> raft_log/dht3_log &

sleep 2

echo 'Creating WooFs'
ssh dht1 'cd ~/cspot1/apps/raft/cspot/; ./create_woofs'
ssh dht2 'cd ~/cspot2/apps/raft/cspot/; ./create_woofs'
ssh dht3 'cd ~/cspot3/apps/raft/cspot/; ./create_woofs'

echo 'Starting Raft cluster'
ssh dht1 'cd ~/cspot1/apps/raft/cspot/; ./start_server -f ../config'
ssh dht2 'cd ~/cspot2/apps/raft/cspot/; ./start_server -f ../config'
ssh dht3 'cd ~/cspot3/apps/raft/cspot/; ./start_server -f ../config'
