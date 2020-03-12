import time
import sh
import sys
import os

logfile = sys.argv[1]
tag = sys.argv[2]
log_tail = sh.tail('-f', logfile, _iter=True)

while True:
    line = log_tail.next().strip()
    if tag in line:
        line = line[line.find(']: ') + 3:]
        print(line)
    time.sleep(0.1)
