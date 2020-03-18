import time
import subprocess
import threading
import os

woof_uri = [
    'woof://10.1.4.138/home/centos/cspot1/apps/dht/cspot',
    'woof://10.1.5.194/home/centos/cspot2/apps/dht/cspot',
    'woof://10.1.4.78/home/centos/cspot3/apps/dht/cspot',
    'woof://10.1.5.4/home/centos/cspot4/apps/dht/cspot',
    'woof://10.1.4.10/home/centos/cspot5/apps/dht/cspot',
    'woof://10.1.5.189/home/centos/cspot6/apps/dht/cspot',
    'woof://10.1.4.249/home/centos/cspot7/apps/dht/cspot',
    'woof://10.1.4.171/home/centos/cspot8/apps/dht/cspot'
]

def get_node_id(woof):
    if 'woof://' in woof:
        return int(woof[-len('/apps/dht/cspot') - 1: -len('/apps/dht/cspot')])
    return None

predecessors = [None for i in range(8)]
successors = [[None for i in range(3)] for j in range(8)]
fingers = [[None for i in range(160)] for j in range(8)]
online = [True for i in range(8)]
info_thread = [None for i in range(8)]

def read_info(i):
    while True:
        online[i] = True
        lines = subprocess.check_output(['ssh', 'dht9', '~/cspot9/apps/dht/node_info -w {}'.format(woof_uri[i])], stderr=subprocess.STDOUT)
        lines = lines.split('\n')
        for line in lines:
            if 'unavailable' in line:
                online[i] = False
                predecessors[i] = None
                successors[i] = [None for i in range(3)]
                fingers[i] = [None for i in range(160)]
            elif 'predecessor_addr' in line:
                woof = line.split(' ')[1]
                predecessors[i] = get_node_id(woof)
            elif 'successor_addr' in line:
                ind = int(line.split(' ')[1][:-1])
                woof = line.split(' ')[2]
                successors[i][ind] = get_node_id(woof)
            elif 'finger_addr' in line:
                ind = int(line.split(' ')[1][:-1]) - 1
                woof = line.split(' ')[2]
                fingers[i][ind] = get_node_id(woof)
    time.sleep(0.5)

for i in range(8):
    info_thread[i] = threading.Thread(target=read_info, args=(i,))
    info_thread[i].daemon = True
    info_thread[i].start()

while True:
    os.system('clear')

    msg = 'Chain: '
    if online[0]:
        msg += ' 1'
        n = 0
        while successors[n][0] != 1:
            msg += ' {}'.format(successors[n][0])
            if successors[n][0] == None:
                break
            n = successors[n][0] - 1
    print(msg + '\n')

    for i in range(8):
        print(i + 1)
        if not online[i]:
            print(' OFFLINE\n')
        else:
            print(' predecessor: {}'.format(predecessors[i]))
            print(' successor: {}'.format(successors[i]))
            # print(' finger: {}'.format(fingers[i]))
    time.sleep(0.1)