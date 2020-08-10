#!/usr/bin/python3

import time
import datetime
import subprocess


def getcmd(src):
    if src == 'pizero_02.csv':
        return 'dht1', '~/combo_cspot1', 'pizero02_cpu'
    elif src == 'pizero_04.csv':
        return 'dht4', '~/combo_cspot4', 'pizero04_cpu'
    elif src == 'pizero_05.csv':
        return 'dht7', '~/combo_cspot7', 'pizero05_cpu'
    elif src == 'pizero_06.csv':
        return 'dht10', '~/combo_cspot10', 'pizero06_cpu'
    elif src == 'pizero_02_dht.csv':
        return 'dht1', '~/combo_cspot1', 'pizero02_dht'
    elif src == 'goleta_pizero02_wu8.csv':
        return 'dht13', '~/combo_cspot13', 'wu_station'


def publish(temp, ts, src):
    try:
        host, nsdir, topic = getcmd(src)
        output = subprocess.check_output(['ssh', host, 'cd {}; ./publish_temp {} {} {}'.format(nsdir, topic, temp, ts)], stderr=subprocess.STDOUT)
        print(output.decode())
    except:
        print('failed to publish')


readings = []
filenames = ['pizero_02.csv', 'pizero_04.csv', 'pizero_05.csv',
             'pizero_06.csv', 'pizero_02_dht.csv', 'goleta_pizero02_wu8.csv']
for filename in filenames:
    with open(filename, 'r') as ifile:
        ifile.readline()
        for line in ifile:
            line = line.strip().split(',')
            if filename == 'goleta_pizero02_wu8.csv':
                dt = datetime.datetime.strptime(line[0], '%Y-%m-%d %H:%M:%S')
            else:
                dt = datetime.datetime.strptime(
                    line[0], '%Y-%m-%d %H:%M:%S.%f')
            readings.append((dt, float(line[1]), filename))

readings.sort()
for r in readings:
    publish(r[1], int(r[0].timestamp() * 1000), r[2])