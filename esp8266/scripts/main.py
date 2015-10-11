# This script is run on boot
import esp
def handler(atask, arg):
    print("in handler\n")
    print(atask)
    if arg:
        print("arg is %r\n" % arg)
    else:
        print("no arg\n")

t = esp.os_task(handler)
pp = "aa = esp.dht(5, task=t)"
