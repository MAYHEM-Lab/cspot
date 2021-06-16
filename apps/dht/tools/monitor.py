import time
import sh
import threading
import os

predecessors = [None] * 8
successors = [None] * 8
fingers = [None] * 8
daemon_logs = [None] * 8
tail_threads = [None] * 8
killed = [False] * 8

for i in range(8):
    daemon_logs[i] = sh.tail('-f', 'dht_log/dht{}_daemon_log'.format(i + 1), _iter=True)

def read_tail(i):
    while True:
        line = daemon_logs[i].next().strip()
        if 'Killed' in line:
            killed[i] = True
        elif '[stablize]: current successor_addr:' in line:
            # successors[i] =  line[line.find('[stablize]: current successor_addr:') + len('[stablize]: current successor_addr:') + 1:]
            # successors[i] = successors[i][-len('/apps/dht/cspot') - 1: -len('/apps/dht/cspot')]
            successors[i] = line[-len('/apps/dht/cspot') - 1: -len('/apps/dht/cspot')]
        elif '[check_predecessor]: current predecessor_addr:' in line:
            # predecessors[i] = line[line.find('[check_predecessor]: current predecessor_addr:') + len('[check_predecessor]: current predecessor_addr:') + 1:]
            # predecessors[i] = predecessors[i][-len('/apps/dht/cspot') - 1: -len('/apps/dht/cspot')]
            if line.strip().endswith('current predecessor_addr:'):
                predecessors[i] = None
            else:
                predecessors[i] = line[-len('/apps/dht/cspot') - 1: -len('/apps/dht/cspot')]
        elif '[finger_addr]: finger_addr' in line:
            # finger = line[line.find('[check_predecessor]: current predecessor_addr:') + len('[check_predecessor]: current predecessor_addr:') + 1:]
            # finger_id = int(line.split(' ')[7][:-1])
            # finger = int(line[-len('/apps/dht/cspot') - 1:-len('/apps/dht/cspot')])
            # index = str(i) + ':' + str(finger_id)
            fingers[i] = line[line.find('[finger_addr]: finger_addr') + len('[finger_addr]: finger_addr') + 1:]
            # print(line[line.find('[finger_addr]: finger_addr') + len('[finger_addr]: finger_addr') + 1:])
        time.sleep(0.1)

for i in range(8):
    tail_threads[i] = threading.Thread(target=read_tail, args=(i,))
    tail_threads[i].daemon = True
    tail_threads[i].start()

while True:
    time.sleep(0.1)
    os.system('clear')

    msg = 'Chain: 1'
    n = 0
    while successors[n] != '1':
        msg += ' {}'.format(successors[n])
        if successors[n] == None:
            break
        n =  int(successors[n]) - 1
    print(msg + '\n')

    for i in range(8):
        print(i + 1)
        if killed[i]:
            print(' KILLED\n')
        else:
            print(' predecessor: {}'.format(predecessors[i]))
            print(' successor: {}'.format(successors[i]))
            # print(' finger: {}'.format(fingers[i]))