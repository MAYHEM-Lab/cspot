import string
import sys

# get request seqno pairs
events = []
with open(sys.argv[1], 'r') as file:
    for line in file:
        if line.startswith('find_result_seq_no'):
            line = line.rstrip().split(', ')
            result_seqno = int(line[0][20:])
            request_seqno = int(line[1][16:])
            hops = int(line[2][5:])
            events.append({'request_seqno': request_seqno, 'result_seqno': result_seqno, 'hops': hops})

# parse cspot log
request_events = []
result_events = []
with open(sys.argv[2], 'r') as file:
    for line in file:
        line = line.strip().split(' ')
        if len(line) != 17:
            continue
            woofc_seq_no = line[10]
            timestamp = line[16]
        elif line[12] == 'dht_find_successor_result':
            result_events.append({'seqno': int(line[10]), 'timestamp': int(line[16])})

with open(sys.argv[3], 'r') as file:
    for line in file:
        # if line[12] == 'dht_find_successor_arg':
        #     request_events.append({'seqno': int(line[10]), 'timestamp': int(line[16])})
        line = line.rstrip().split(', ')
        if len(line) == 2:
            request_events.append({'seqno': int(line[0]), 'timestamp': int(line[1])})

for event in events:
    request_seqno = event['request_seqno']
    result_seqno = event['result_seqno']
    for request_event in request_events:
        if request_event['seqno'] == request_seqno:
            request_timestamp = request_event['timestamp']
    for result_event in result_events:
        if result_event['seqno'] == result_seqno:
            result_timestamp = result_event['timestamp']
    print('request_seqno: {}, result_seqno: {}'.format(request_seqno, result_seqno))
    print('request_timestamp: {}, result_timestamp: {}'.format(request_timestamp, result_timestamp))
    print('hops: {}, diff: {}'.format(event['hops'], result_timestamp - request_timestamp))