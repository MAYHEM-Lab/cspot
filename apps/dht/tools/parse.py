import string
import os.path
import sys

def select_type(events, type):
    result = []
    for event in events:
        if event['type'] == string.upper(type):
            result.append(event)
    return result

def select_host(events, host):
    result = []
    for event in events:
        if event['host'] == host:
            result.append(event)
    return result

def select_function(events, function):
    result = []
    for event in events:
        if event['function'] == function:
            result.append(event)
    return result

events = []
with open(sys.argv[1], 'r') as file:
    # for logname in ['log_host', 'log_daemon', 'log_create', 'log_join']:
        # filename = '{}/{}'.format(host, logname)
        # if os.path.exists(filename):
        #     with open(filename, 'r') as file:
    for line in file:
        if len(line) > 5 and line[5] == '|':
            type = line[:5].rstrip()
            timestamp = line[7:26]
            right_bkt = line.index(']')
            function = line[28:right_bkt]
            text = line[right_bkt + 3:].rstrip()
            events.append({'type': type, 'timestamp': timestamp, 'function': function, 'text': text})

# events = select_host(events, 'cspot-dev2')
l = []
events = select_function(events, 'find_result')

for event in events:
    print(event['text'])
    
# for event in events_find_result:
#     print(event)