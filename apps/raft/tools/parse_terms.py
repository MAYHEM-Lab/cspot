import sys

def get_timestamp(ts):
    ts = ts.split(':')
    ts = [int(i) for i in ts]
    return ts[0] * 3600000 + ts[1] * 60000 + ts[2] * 1000 + ts[3]

leaders = {}
with open(sys.argv[1], 'r') as logfile:
    for line in logfile:
        timestamp = line.split(' ')[0]
        term = int(line.split(' ')[2][:-1])
        leader = int(line.split(' ')[3].strip())
        leaders[term] = (leader, timestamp)

last_ts = 0
for i in range(100):
    if i not in leaders:
        print('Term {}:   '.format(i))
    else:
        ts = get_timestamp(leaders[i][1])
        print('Term {}: {} {}'.format(i, leaders[i][0], ts - last_ts))
        last_ts = ts