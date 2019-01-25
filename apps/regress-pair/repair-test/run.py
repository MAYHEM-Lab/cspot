import os
import time
import subprocess

woof_name = 'test'
countback = 20
history_size = 3000
lags = 12
cmd = "./regress-pair-init -W {} -c {} -s {} -l {}".format(woof_name, countback, history_size, lags)
print(cmd)
os.system(cmd)

datapath = os.path.dirname(os.path.realpath(__file__))
with open(datapath + '/data/4_cpu', 'r') as mfile:
    with open(datapath + '/data/4_dht', 'r') as pfile:
        mdata = [line.rstrip('\n').split(', ') for line in mfile]
        pdata = [line.rstrip('\n').split(', ') for line in pfile]

head = 1542265200
missed = range(1542438000, 1542524400)
mi = 0
pi = 0
seq_no = []
# while mi < len(mdata) or pi < len(pdata):
while mi < 30 or pi < 30:
    while mi < len(mdata):
        date, ts, value = mdata[mi]
        ts = int(ts)
        value = float(value)
        if head >= ts:
            if ts in missed:
                cmd = "echo {} {} | ./regress-pair-put -W {} -L -s 'm'".format(ts, last_value, woof_name)
            else:
                cmd = "echo {} {} | ./regress-pair-put -W {} -L -s 'm'".format(ts, value, woof_name)
                last_value = value
            print(cmd)
            if ts in missed:
                seq_no += [subprocess.check_output(cmd, shell=True)]
            else:
                os.system(cmd)
            time.sleep(0.1)
            mi += 1
        else:
            break
    while pi < len(pdata):
        date, ts, value = pdata[pi]
        ts = int(ts)
        value = float(value)
        if head >= ts:
            cmd = "echo {} {} | ./regress-pair-put -W {} -L -s 'p'".format(ts, value, woof_name)
            print(cmd)
            os.system(cmd)
            time.sleep(0.1)
            pi += 1
        else:
            break
    head += 60
print(seq_no)