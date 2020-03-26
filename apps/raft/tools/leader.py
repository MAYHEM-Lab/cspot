import time
import sh
import threading
import os

NUM_REPLICAS = 3

namespace_logs = [None] * NUM_REPLICAS
tail_threads = [None] * NUM_REPLICAS

for i in range(NUM_REPLICAS):
    namespace_logs[i] = sh.tail('-f', 'raft_log/dht{}_log'.format(i + 1), _iter=True)

def read_tail(i):
    while True:
        time.sleep(0.01)
        line = namespace_logs[i].next().strip()
        if 'promoted to leader' in line:
            term = line.split(' ')[-1]
            print('Term {}: {}'.format(term, i + 1))

for i in range(NUM_REPLICAS):
    tail_threads[i] = threading.Thread(target=read_tail, args=(i,))
    tail_threads[i].daemon = True
    tail_threads[i].start()

while True:
    time.sleep(0.1)
    # pass