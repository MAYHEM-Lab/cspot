import os
import time
import subprocess

delay = 0.05
woof_name = 'test'
countback = 10
history_size = 30000
lags = 10
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
seq_no_repair = []
m_cmds = []
p_cmds = []

# mdata, pdata = mdata[:30], pdata[:30]
 
while mi < len(mdata) or pi < len(pdata):
    woof_name = "woof://10.1.5.1:51374/home/centos/cspot/apps/regress-pair/cspot/test"
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
            # raw_input("Press Enter to continue...")
            print(cmd)
            # seq_no = int(subprocess.check_output(cmd, shell=True))
            seq_no = subprocess.check_output(cmd, shell=True)
            print(seq_no)
            if ts in missed:
                seq_no_repair += [seq_no]
            time.sleep(delay)
            mi += 1
        else:
            break
    woof_name = "woof://10.1.5.155:55376/home/centos/cspot2/apps/regress-pair/cspot/test"
    while pi < len(pdata):
        date, ts, value = pdata[pi]
        ts = int(ts)
        value = float(value)
        if head >= ts:
            cmd = "echo {} {} | ./regress-pair-put -W {} -L -s 'p'".format(ts, value, woof_name)
            # raw_input("Press Enter to continue...")
            print(cmd)
            # seq_no = int(subprocess.check_output(cmd, shell=True))
            seq_no = subprocess.check_output(cmd, shell=True)
            print(seq_no)
            time.sleep(delay)
            pi += 1
        else:
            break
    head += 60

print("need to be repaired:")
print(seq_no_repair)

print("run this line to repair:")
print("~/cspot/woof-repair -H 10.1.5.1:51374,10.1.5.155:55376 -F glog -W woof://10.1.5.1:51374/home/centos/cspot/apps/regress-pair/cspot/test -O guide -S {}".format(",".join([str(i) for i in seq_no_repair])))