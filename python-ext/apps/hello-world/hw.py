import json
import time

def python_handler(woof, seq_no, data):
	print('Handler triggered: {0}'.format(int(time.time() * 1000)))
	print('WooF: {0}'.format(woof))
	print('Seq_no: {0}'.format(seq_no))
	print('Raw data: {0}'.format(data))
	print('Unmarshalled:\n{0}'.format(json.loads(data)))
