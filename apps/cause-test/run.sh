PREV=${PWD}
cd cspot; ../create -F test
../api-send -W woof://10.1.5.155:56554/home/centos/cspot2/apps/cause-test/cspot/test -F test
../api-send -W woof://10.1.5.155:56554/home/centos/cspot2/apps/cause-test/cspot/test -F test
../api-send -W woof://10.1.5.155:56554/home/centos/cspot2/apps/cause-test/cspot/test -F test
../api-send -W woof://10.1.5.155:56554/home/centos/cspot2/apps/cause-test/cspot/test -F test
../api-send -W woof://10.1.5.155:56554/home/centos/cspot2/apps/cause-test/cspot/test -F test
# ../api-send -W woof://10.1.5.155:56554/home/centos/cspot2/apps/cause-test/cspot/test -F test
# ../api-send -W woof://10.1.5.155:56554/home/centos/cspot2/apps/cause-test/cspot/test -F test
# ../api-send -W woof://10.1.5.155:56554/home/centos/cspot2/apps/cause-test/cspot/test -F test
# ../api-send -W woof://10.1.5.155:56554/home/centos/cspot2/apps/cause-test/cspot/test -F test
# ../api-send -W woof://10.1.5.155:56554/home/centos/cspot2/apps/cause-test/cspot/test -F test
cd ../../merge-test; make clean; make; cd -; cp ../../merge-test/cspot/merge-init  ./
./merge-init -E 10.1.5.155:56554,10.1.5.1:55064 -F glog
cd ${PREV}