class LogParser(object):
    def __init__(self, filename):
        self.filename = filename
        self.status = ['INVALID']
        self.skip = False

    def parse(self):
        version_stamp = 0
        num_of_entries_accessed = 0
        with open(self.filename) as fh:
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
                    print('INSERT VS:{} ENTRIES:{}'.format(version_stamp, num_of_entries_accessed))
                    continue
                if line.startswith('DELETE START'):
                    self.status.append('DELETE')
                    num_of_entries_accessed = 0
                    version_stamp = int(line.split(':')[1])
                    continue
                if line.startswith('DELETE END'):
                    self.status.pop()
                    print('DELETE VS:{} ENTRIES:{}'.format(version_stamp, num_of_entries_accessed))
                    continue
                if line.startswith('DATA') or line.startswith('LINK'):
                    num_of_entries_accessed += 1
                    continue

if __name__ == '__main__':
    lp = LogParser("cspot/remote-result.log")
    lp.parse()
