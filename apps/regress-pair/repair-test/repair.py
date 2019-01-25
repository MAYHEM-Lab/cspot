import os
import time
import subprocess

woof_name = 'test'
countback = 20
history_size = 3000
lags = 12

datapath = os.path.dirname(os.path.realpath(__file__))
with open(datapath + '/data/4_cpu', 'r') as mfile:
    mdata = [line.rstrip('\n').split(', ') for line in mfile]

head = 1542265200
missed = range(1542438000, 1542524400)
mi = 0
cnt = 0
seq_no = []
while mi < len(mdata):
    while mi < len(mdata):
        date, ts, value = mdata[mi]
        ts = int(ts)
        value = float(value)
        if head >= ts:
            if ts in missed:
                cmd = "echo {} {} | ./regress-pair-put -W {} -L -s 'm'".format(ts, value, woof_name)
                print(cmd)
                seq_no += [subprocess.check_output(cmd, shell=True)]
                cnt += 1
                time.sleep(0.1)
            mi += 1
        else:
            break
    head += 60
print(seq_no)
print("total repaired: {}".format(cnt))