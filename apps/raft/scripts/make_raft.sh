ssh dht1 'cd ~/cspot1/; make abd; cd apps/raft/; make clean; make' &
ssh dht2 'cd ~/cspot2/; make abd; cd apps/raft/; make clean; make' &
ssh dht3 'cd ~/cspot3/; make abd; cd apps/raft/; make clean; make'

sleep 1

