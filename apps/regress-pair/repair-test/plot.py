import matplotlib.pyplot

errors_ts = []
errors_value = []
errors_repaired_ts = []
errors_repaired_value = []
with open('errors', 'r') as file:
    for line in file:
        line = line.rstrip('\n').split()
        errors_ts += [float(line[0])]
        if abs(float(line[1])) > 100:
            errors_value += [10.0]
        else:
            errors_value += [float(line[1])]
with open('errors_repaired', 'r') as file:
    for line in file:
        line = line.rstrip('\n').split()
        errors_repaired_ts += [float(line[0])]
        if abs(float(line[1])) > 100:
            errors_repaired_value += [10.0]
        else:
            errors_repaired_value += [float(line[1])]

matplotlib.pyplot.plot(errors_ts, errors_value, color='red', label='corrupted')
matplotlib.pyplot.plot(errors_repaired_ts, errors_repaired_value, color='blue', label='repaired')
matplotlib.pyplot.show()