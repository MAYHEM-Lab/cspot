import sys
sys.path.insert(0, '../../..')
import woof
import json

woof.WooFInit()
woof.WooFCreate('test', 128, 5)

woofname = 'test'
handler = 'hw'
data = {'key': 'value'}

seq = woof.WooFPut(woofname, handler, json.dumps(data))
print('Put data to WooF at {0}'.format(seq))

result = woof.WooFGet(woofname, seq)
print('Get data at {0}: {1}'.format(seq, result[1]))
