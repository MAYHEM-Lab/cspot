class LogParser(object):
    def __init__(self, log_filename, display_filename):
        self.log_filename = log_filename
        self.display_filename = display_filename
        self.status = ['INVALID']
        self.skip = False

    def get_cumulative_woof_access(self):
        version_stamp = 0
        num_of_entries_accessed = 0
        running_num = 0
        cumulative_num = []
        with open(self.log_filename) as fh:
            lines = fh.readlines()
            for line in lines:
                line = line.strip()
                if line.startswith('populate_terminal_node START'):
                    self.skip = True
                    continue
                if line.startswith('populate_terminal_node END'):
                    self.skip = False
                    continue
                if self.skip:
                    continue
                if line.startswith('INSERT START'):
                    self.status.append('INSERT')
                    num_of_entries_accessed = 0
                    version_stamp = int(line.split(':')[1])
                    continue
                if line.startswith('INSERT END'):
                    self.status.pop()
                    #print('INSERT VS:{} ENTRIES:{}'.format(version_stamp, num_of_entries_accessed))
                    running_num += num_of_entries_accessed;
                    cumulative_num.append(running_num)
                    continue
                if line.startswith('DELETE START'):
                    self.status.append('DELETE')
                    num_of_entries_accessed = 0
                    version_stamp = int(line.split(':')[1])
                    continue
                if line.startswith('DELETE END'):
                    self.status.pop()
                    #print('DELETE VS:{} ENTRIES:{}'.format(version_stamp, num_of_entries_accessed))
                    running_num += num_of_entries_accessed;
                    cumulative_num.append(running_num)
                    continue
                if line.startswith('DATA') or line.startswith('LINK'):
                    num_of_entries_accessed += 1
                    continue
        return cumulative_num

    def get_cumulative_update_step_old(self):
        vs = 1
        num_of_ops = 0
        cumulative_num = []
        with open(self.display_filename) as fh:
            lines = fh.readlines()
            prev_vals = []
            for line in lines:
                div = line.split(':')
                vs = int(div[0].strip())
                numbers = div[1].split(' ')
                vals = []
                for number in numbers:
                    number = number.strip()
                    if number != '':
                        vals.append(int(number))
                if len(vals) > len(prev_vals):
                    num_of_ops += 1
                else:
                    for x in range(len(prev_vals)):
                        if x == len(vals):
                            num_of_ops += x
                            break
                        if(prev_vals[x] != vals[x]):
                            num_of_ops += (x + 1)
                            break
                cumulative_num.append(num_of_ops)
                prev_vals = vals
                vs += 1
        return cumulative_num
    
    def get_cumulative_update_step(self):
        cumulative_num = []
        running_num = 0
        with open('cspot/foo1', 'r') as fh:
            lines = fh.readlines()
            for line in lines:
                running_num += int(line.strip())
                cumulative_num.append(running_num)
        return cumulative_num

    def debug(self):
        wa = self.get_cumulative_woof_access()
        us = self.get_cumulative_update_step()
        for (a, b) in zip(wa, us):
            print('{},{}'.format(b, a))


if __name__ == '__main__':
    lp = LogParser('cspot/remote-result.log', 'workload-display.txt')
    lp.debug()
