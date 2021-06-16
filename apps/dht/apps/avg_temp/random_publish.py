import random
import subprocess
import time

while True:
    temp = str(random.uniform(75.0, 76.0))
    subprocess.check_output(['./publish_temp', temp], stderr=subprocess.STDOUT)
    time.sleep(60)