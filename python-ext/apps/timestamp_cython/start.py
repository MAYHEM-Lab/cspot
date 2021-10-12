import sys
sys.path.insert(0, '../../..')
from woof import *
import json

woofname = 'test'
handler = 'ts'
data = {}

WooFInit()
WooFCreate(woofname, 4232, 5)

data['stop'] = 3
data['ts'] = []
data['head'] = 0
data['woof'] = []
data['woof'] += [woofname]
data['woof'] += [woofname]
data['woof'] += [woofname]

ndx = WooFPut(woofname, handler, json.dumps(data))
print('Put data to WooF at {0}'.format(ndx))
