import time
import threading
import signal

NUM_REPLICAS = 3
active = [True] * NUM_REPLICAS
tail_threads = [None] * NUM_REPLICAS

def main():
    signal.signal(signal.SIGINT, handler)
    for i in range(NUM_REPLICAS):
        tail_threads[i] = threading.Thread(target=read_tail, args=(i,))
        tail_threads[i].daemon = True
        tail_threads[i].start()
    i = 1
    while True:
        time.sleep(0.1)
        pass

def read_tail(i):
    global active
    logfile = open('raft_log/dht{}_log'.format(i + 1), 'r')
    logfile.seek(0, 2)
    while True:
        if not active[i]:
            break
        line = logfile.readline()
        if not line:
            time.sleep(0.1)
            continue
        if 'role: LEADER' in line:
            term = int(line.strip().split(' ')[-3][:-1])
            timestamp = ' '.join(line.strip().split(' ')[3:7])
            print('[{}] Term {}: {}'.format(timestamp, term, i + 1))
    logfile.close()

def handler(sig, frame):
    if sig == signal.SIGINT:
        cleanup()
    exit(0)

def cleanup():
    for i in range(NUM_REPLICAS):
        active[i] = False
        tail_threads[i].join()

if __name__ == '__main__':
    main()