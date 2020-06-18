import sys

start = 1542265200
end = 1542870000
print('expected lines: {}'.format((end - start) / 300))

cnt = 0
missed = 0
last_ts = start
last_temp = 0
with open(sys.argv[1], 'r') as ifile:
    with open(sys.argv[2], 'w') as ofile:
        for line in ifile:
            date, ts, temp = line.strip().split(', ')
            ts = int(float(ts) / 10) * 10
            temp = float(temp)
            if ts < start or ts >= end:
                last_temp = temp
                last_ts = ts
                continue
            while ts - last_ts > 400:
                cnt += 1
                missed += 1
                ofile.write('{}                    , {}, {}\n'.format('missed', last_ts + 300, last_temp))
                last_ts += 300
            cnt += 1
            ofile.write('{}, {}, {}\n'.format(date, ts, temp))
            last_temp = temp
            last_ts = ts

print('totally written: {}'.format(cnt))
print('missed: {}'.format(missed))