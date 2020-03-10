import time
import sh
import threading
import os

predecessors = [None] * 8
successors = [None] * 8
daemon_logs = [None] * 8
tail_threads = [None] * 8
for i in range(8):
    daemon_logs[i] = sh.tail('-f', 'dht_log/dht{}_daemon_log'.format(i + 1), _iter=True)

def read_tail(i):
    while True:
        line = daemon_logs[i].next().strip()
        if '[stablize]: current successor_addr:' in line:
            successors[i] =  line[line.find('[stablize]: current successor_addr:') + len('[stablize]: current successor_addr:') + 1:]
            successors[i] = successors[i][-len('/apps/dht/cspot') - 1: -len('/apps/dht/cspot')]
        elif '[check_predecessor]: current predecessor_addr:' in line:
            predecessors[i] = line[line.find('[check_predecessor]: current predecessor_addr:') + len('[check_predecessor]: current predecessor_addr:') + 1:]
            predecessors[i] = predecessors[i][-len('/apps/dht/cspot') - 1: -len('/apps/dht/cspot')]
        time.sleep(0.1)

for i in range(8):
    tail_threads[i] = threading.Thread(target=read_tail, args=(i,))
    tail_threads[i].daemon = True
    tail_threads[i].start()

while True:
    time.sleep(0.1)
    os.system('clear')
    for i in range(8):
        print(i + 1)
        print('  predecessor: {}'.format(predecessors[i]))
        print('  successor: {}'.format(successors[i]))