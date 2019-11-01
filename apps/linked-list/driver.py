from os import listdir
from os.path import join
from subprocess import Popen

class Driver(object):
    def __init__(self, workload_dir, display_dir):
        self.workload_dir = workload_dir
        self.display_dir = display_dir

    def get_workload_filenames(self):
        return sorted(listdir(self.workload_dir))

    def generate_timing_space_woof_access_steps(self):
        filenames = self.get_workload_filenames()
        for filename in filenames:
            filename = join(self.workload_dir, filename)
            process = Popen(['./app', join('..', filename)], cwd='./cspot')
            process.wait()

    def generate_display(self):
        filenames = self.get_workload_filenames()
        for filename in filenames:
            suffix = filename.split('-')[1].split('.')[0]
            output_filename = join(self.display_dir, 'remote-display-{}.log'.format(suffix))
            filename = join(self.workload_dir, filename)
            with open(output_filename, 'wb') as fh:
                process = Popen(['./app', join('..', filename)], stdout=fh, cwd='./cspot')
            process.wait()

if __name__ == '__main__':
    d = Driver('workloads', 'displays')
    d.generate_timing_space_woof_access_steps()

