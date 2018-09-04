import sys
sys.path.insert(0, '/cspot')
from woof import *
import json
import time

def timestamp():
	return int(time.time() * 1000)

def python_handler(woof, seq_no, data):
	ts = timestamp()
	print('timestamp: {0}'.format(ts))
	print('from woof {0} at {1}'.format(woof['filename'], seq_no))
	data = json.loads(data)
	data['ts'] += [ts]
	data['head'] += 1
	
	if data['head'] == data['stop']:
		print('Reach the last stop')
		for i in range(data['stop']):
			print('timestamp of {0}: {1}'.format(data['woof'][i], data['ts'][i]))
	else:
		print('next WooF: {0}'.format(data['woof'][data['head']]))
		result = WooFPut(data['woof'][data['head']], 'ts', json.dumps(data));

