./create

for i in {1..1000}
do
    echo ${i}
    ./find -n woof://10.1.4.138:/home/centos/cspot1/apps/dht/cspot -t topic${i}
    usleep 20000
done