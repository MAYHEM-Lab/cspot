from os import listdir
from os.path import join
from subprocess import Popen

class Driver(object):
    def __init__(self, workload_dir, display_dir):
        self.workload_dir = workload_dir
        self.display_dir = display_dir

    def generate_display(self):
        filenames = sorted(listdir(self.workload_dir))
        i = 0
        for x in range(50):
            filenames.pop(0)
        for filename in filenames:
            if i == 50:
                break
            suffix = filename.split('-')[1].split('.')[0]
            output_filename = join(self.display_dir, 'remote-display-{}.log'.format(suffix))
            filename = join(self.workload_dir, filename)
            with open(output_filename, 'wb') as fh:
                process = Popen(['./app', join('..', filename)], stdout=fh, cwd='./cspot')
            process.wait()
            i += 1

if __name__ == '__main__':
    d = Driver('workloads', 'displays')
    d.generate_display()

