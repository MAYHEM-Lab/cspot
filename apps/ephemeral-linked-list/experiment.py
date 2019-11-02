import os
import subprocess

class Experiment(object):
    def __init__(self):
        pass
    
    def runTiming(self):
        for i in range(100, 1001, 100):
            process = subprocess.Popen(['./app', str(i)], cwd=self.getWorkingDirectory())
            process.wait()

    def getWorkingDirectory(self):
        return os.path.join(os.getcwd(), 'cspot')

if __name__ == '__main__':
    exp = Experiment()
    exp.runTiming()
