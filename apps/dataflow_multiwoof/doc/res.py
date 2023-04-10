import statistics

filename = 'log.txt'

start = []
end = []
latency = []
throughput = []

# Parse raw output (and convert from ns to ms)
with open(filename, 'r') as f:
    for line in f:
        if line == 'stop\n':
            break
        if line.startswith('start:') or line.startswith('end'):

        # Format: [start|end]: [timestamp]ns 
            start_or_end, value = line.split(' ')
            if start_or_end.startswith('start:'):
                start.append(int(value.rstrip()[:-2]) * 1e-6)
            elif start_or_end.startswith('end'):
                end.append(int(value.rstrip()[:-2]) * 1e-6)


print(f'start: {len(start)}, end: {len(end)}')
print(f'{len(start)} runs')

# Calculate latency
for s, e in zip(start, end):
    latency.append(e - s)

# Calculate throughput
for i in range(len(end) - 1):
    throughput.append(1 / ((end[i + 1] - end[i]) * 1e-3))

# # Calculate throughput (of each pair of ends)
#for i in range(0, len(end) - 1, 2):
#    if(1 / ((end[i + 1] - end[i]) * 1e-3) < 300):
#        throughput.append(1 / ((end[i + 1] - end[i]) * 1e-3))

print('\nLatency:')
for i, l in enumerate(latency):
    print(f'{l}')
latency.sort()
print(f'Average: {sum(latency) / len(latency)} ms')
print(f'Median: {statistics.median(latency)} ms')
print(f'Min: {min(latency)} ms')
print(f'Max: {max(latency)} ms')
median_index = latency.index(latency[len(latency)//2])
print(f'Quartile1: {statistics.median(latency[:median_index + 1])}')
print(f'Quartile3: {statistics.median(latency[median_index:])}')

print('\nThroughput:')
for t in throughput:
    print(f'{t} outputs/s')
throughput.sort()
print(f'Average: {sum(throughput) / len(throughput)} outputs/s')
print(f'Median: {statistics.median(throughput)} outputs/s')
print(f'Min: {min(throughput)} outputs/s')
print(f'max: {max(throughput)} outputs/s\n')
median_index = throughput.index(throughput[len(throughput)//2])
print(f'Quartile1: {statistics.median(throughput[:median_index + 1])}')
print(f'Quartile3: {statistics.median(throughput[median_index:])}')