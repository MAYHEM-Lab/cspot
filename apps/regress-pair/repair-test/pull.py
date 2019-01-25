import sys
import subprocess

ts = []
value = []
for i in range(int(sys.argv[2])):
    cmd = './regress-pair-get -W test -L -s {} -S {}'.format(sys.argv[1], i + 1)
    out = subprocess.check_output(cmd.split()).split()
    ts += [float(out[2])]
    value += [float(out[0])]
    print('{} {}'.format(out[2], out[0]))
