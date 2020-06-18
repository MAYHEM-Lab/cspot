import os
import time
import subprocess

woof_name = 'test'
countback = 10
history_size = 30000
lags = 10
cmd = "./regress-pair-init -W {} -c {} -s {} -l {}".format(woof_name, countback, history_size, lags)
print(cmd)
os.system(cmd)
