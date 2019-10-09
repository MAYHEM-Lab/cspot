import os
import time
import subprocess

delay = 0.05
woof_name = 'test'
countback = 20
history_size = 30000
lags = 12

datapath = os.path.dirname(os.path.realpath(__file__))
with open('guide', 'r') as gfile:
    gdata = [line.rstrip('\n').split(',') for line in gfile]
with open(datapath + '/data/4_cpu', 'r') as mfile:
    mdata = [line.rstrip('\n').split(', ') for line in mfile]
with open(datapath + '/data/4_dht', 'r') as pfile:
    pdata = [line.rstrip('\n').split(', ') for line in pfile]

head = 1542265200
mi = 0
pi = 0
m_cmds = ['']
p_cmds = ['']
 
while mi < len(mdata) or pi < len(pdata):
    woof_name = "woof://10.1.5.1:51374/home/centos/cspot/apps/regress-pair/cspot/test"
    while mi < len(mdata):
        date, ts, value = mdata[mi]
        ts = int(ts)
        value = float(value)
        if head >= ts:
            cmd = "echo {} {} | ./regress-pair-put -W {} -L -s 'm'".format(ts, value, woof_name)
            # print(cmd)
            m_cmds += [cmd]
            # seq_no = int(subprocess.check_output(cmd, shell=True))
            # print(seq_no)
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
            # print(cmd)
            p_cmds += [cmd]
            # seq_no = int(subprocess.check_output(cmd, shell=True))
            # print(seq_no)
            pi += 1
        else:
            break
    head += 60

# for i in gdata:
#     host = i[0]
#     if host == '7886325280287311374':
#         cmd = m_cmds[int(i[2])]
#     if host == '797386831364045376':
#         cmd = p_cmds[int(i[2])]
#     print(cmd)
#     seq_no = subprocess.check_output(cmd, shell=True)
#     time.sleep(delay)
#     print("cmd[{}]: {}".format(int(i[2]), seq_no))

for i in range(577, 1152+1):
    cmd = m_cmds[i]
    print(cmd)
    seq_no = subprocess.check_output(cmd, shell=True)
    time.sleep(delay)
    print(seq_no)

    cmd = p_cmds[i]
    print(cmd)
    seq_no = subprocess.check_output(cmd, shell=True)
    time.sleep(delay)
    print(seq_no)