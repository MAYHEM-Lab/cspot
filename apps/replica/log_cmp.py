import string
import sys

# get request seqno pairs
master_events = []
slave_events = []
with open(sys.argv[1], 'r') as master_log:
    with open(sys.argv[2], 'r') as slave_log:
        for i in range(4):
            master_log.readline()
            slave_log.readline()
        for line in master_log:
            line = line.rstrip().split()
            master_event = {'seq_no': line[4], 'woofc_seq_no': line[10], 'woof': line[12], 'type': line[14]}
            line = slave_log.readline().rstrip().split()
            slave_event = {'seq_no': line[4], 'woofc_seq_no': line[10], 'woof': line[12], 'type': line[14]}
            if master_event != slave_event:
                print('log not equal')
                exit(1)
print('log equal')