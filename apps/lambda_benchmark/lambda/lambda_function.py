import os
import sys
import ctypes
import base64

os.environ['WOOFC_NAMESPACE'] = 'WOOFC_NAMESPACE'
os.environ['WOOF_HOST_IP'] = 'WOOF_HOST_IP'
os.environ['WOOFC_DIR'] = 'WOOFC_DIR'
os.environ['WOOF_NAMELOG_DIR'] = 'WOOF_NAMELOG_DIR'
os.environ['WOOF_SHEPHERD_NAME'] = 'WOOF_SHEPHERD_NAME'
os.environ['WOOF_SHEPHERD_NDX'] = 'WOOF_SHEPHERD_NDX'
os.environ['WOOF_SHEPHERD_SEQNO'] = 'WOOF_SHEPHERD_SEQNO'
os.environ['WOOF_NAMELOG_NAME'] = 'WOOF_NAMELOG_NAME'
os.environ['WOOF_NAMELOG_SEQNO'] = 'WOOF_NAMELOG_SEQNO'
os.environ['WOOF_NAME_ID'] = 'WOOF_NAME_ID'

cwd = os.getcwd()
_hw = ctypes.CDLL(cwd + '/libhw.so')
#_hw.main.argtypes = (ctypes.c_int, ctypes.POINTER(ctypes.POINTER(ctypes.c_char)), ctypes.POINTER(ctypes.POINTER(ctypes.c_char)))
_hw.main.argtypes = (ctypes.c_int, ctypes.c_char_p, ctypes.c_void_p)

def lambda_handler(event, context):
    if event != None and 'Records' in event:
        for record in event['Records']:
            if record['eventName'] == 'INSERT':
                print('Key: {0}'.format(record['dynamodb']['Keys']['Id']['S']))
                filename = record['dynamodb']['Keys']['Id']['S']
                os.environ['DDB_KEY'] = filename
                print('Content: {0}'.format(record['dynamodb']['NewImage']['Data']['B']))
                data = base64.b64decode(record['dynamodb']['NewImage']['Data']['B'])

                argc = len(sys.argv)
                err = _hw.main(argc, filename, data)
                print(err)
                return str(err)

