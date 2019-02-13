import os
import time
import subprocess

woof_name = 'test'
countback = 20
history_size = 30000
lags = 12

datapath = os.path.dirname(os.path.realpath(__file__))
with open('guide', 'r') as gfile:
    gdata = [int(line.rstrip('\n').split(',')[2]) for line in gfile]
with open(datapath + '/data/4_cpu', 'r') as mfile:
    mdata = [line.rstrip('\n').split(', ') for line in mfile]
with open(datapath + '/data/4_dht', 'r') as pfile:
    pdata = [line.rstrip('\n').split(', ') for line in pfile]

head = 1542265200
mi = 0
pi = 0
cmds = ['']
 
while mi < len(mdata) or pi < len(pdata):
    while mi < len(mdata):
        date, ts, value = mdata[mi]
        ts = int(ts)
        value = float(value)
        if head >= ts:
            cmd = "echo {} {} | ./regress-pair-put -W {} -L -s 'm'".format(ts, value, woof_name)
            # print(cmd)
            cmds += [cmd]
            # seq_no = int(subprocess.check_output(cmd, shell=True))
            # print(seq_no)
            mi += 1
        else:
            break
    while pi < len(pdata):
        date, ts, value = pdata[pi]
        ts = int(ts)
        value = float(value)
        if head >= ts:
            cmd = "echo {} {} | ./regress-pair-put -W {} -L -s 'p'".format(ts, value, woof_name)
            # print(cmd)
            cmds += [cmd]
            # seq_no = int(subprocess.check_output(cmd, shell=True))
            # print(seq_no)
            pi += 1
        else:
            break
    head += 60

for i in gdata:
    cmd = cmds[i]
    print(cmd)
    seq_no = subprocess.check_output(cmd, shell=True)
    time.sleep(0.1)
    print("cmd[{}]: {}".format(i, seq_no))