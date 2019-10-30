import string
import sys

# get request seqno pairs
events = []
min_ts = 2000000000000
max_ts = 0
with open(sys.argv[1], 'r') as log:
    for i in range(4):
        log.readline()
    for line in log:
        line = line.rstrip().split()
        event = {'seq_no': line[4], 'woofc_seq_no': line[10], 'woof': line[12], 'type': line[14]}
        min_ts = min(min_ts, int(line[16]))
        max_ts = max(max_ts, int(line[16]))
        events += [event]
print('number of events: {}'.format(len(events)))
print('elapsed: {}'.format(max_ts - min_ts))