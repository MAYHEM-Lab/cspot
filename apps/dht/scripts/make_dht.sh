ssh dht1 'cd ~/cspot1/; make abd; cd apps/dht/; make clean; make' &
ssh dht2 'cd ~/cspot2/; make abd; cd apps/dht/; make clean; make' &
ssh dht3 'cd ~/cspot3/; make abd; cd apps/dht/; make clean; make' &
ssh dht4 'cd ~/cspot4/; make abd; cd apps/dht/; make clean; make' &
ssh dht5 'cd ~/cspot5/; make abd; cd apps/dht/; make clean; make' &
ssh dht6 'cd ~/cspot6/; make abd; cd apps/dht/; make clean; make' &
ssh dht7 'cd ~/cspot7/; make abd; cd apps/dht/; make clean; make' &
ssh dht8 'cd ~/cspot8/; make abd; cd apps/dht/; make clean; make' &

sleep 8
# ssh dht9 'cd ~/cspot9/; make abd; cd apps/dht/; make clean; make'
