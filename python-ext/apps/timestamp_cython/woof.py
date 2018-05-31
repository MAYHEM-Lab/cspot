import os
import ctypes

container = os.path.exists('/.dockerenv')

if container:
    _woof = ctypes.CDLL('/cspot/libwoof.so')
else:
    cwd = os.getcwd()
    _woof = ctypes.CDLL(cwd + '/libwoof.so')
_woof.WooFInitEnv.argtypes = []
_woof.WooFInit.argtypes = []
_woof.WooFCreate.argtypes = [ctypes.c_char_p, ctypes.c_ulong, ctypes.c_ulong]
_woof.WooFPut.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_void_p]
_woof.WooFGet.argtypes = [ctypes.c_char_p, ctypes.c_void_p, ctypes.c_ulong]
_woof.WooFGetLatestSeqno.argtypes = [ctypes.c_char_p]

if container:
    _woof.WooFInitEnv()

def WooFInit():
    global _woof
    result = _woof.WooFInit()
    return int(result)

def WooFCreate(name, element_size, history_size):
    global _woof
    result = _woof.WooFCreate(name, element_size, history_size)
    return int(result)

def WooFPut(wf_name, hand_name, element):
    global _woof
    result = _woof.WooFPut(wf_name, hand_name, element)
    return int(result)

def WooFGet(wf_name, seq_no):
    global _woof
    element = ctypes.create_string_buffer(1024)
    result = _woof.WooFGet(wf_name, element, seq_no)
    if int(result) == 1:
        return 1, element.value
    else:
        return int(result), None

def WooFGetLatestSeqno(name):
    global _woof
    result = _woof.WooFGetLatestSeqno(name)
    return int(result)
