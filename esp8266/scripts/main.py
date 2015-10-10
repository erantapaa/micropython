# This script is run on boot
import esp
t = esp.os_task(lambda a: print("in task"))
pp = "aa = esp.dht(5, task=t)"
