import subprocess

class Experiment(object):
    def __init__(self):
        pass
    
    def run(self):
        for i in range(100, 1001, 100):
            process = subprocess.Popen(['./app', str(i)])
            process.wait()

if __name__ == '__main__':
    exp = Experiment()
    exp.run()
