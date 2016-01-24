import gc
import esp

counter = 1

def cb(aws):
    global counter
    counter = counter + 1
#    print("got aws %r" % counter)
#    print(aws.headers())
#    print(aws.uri())
#    print(aws.method())
    return '{"status": %d}' % counter

aa = esp.ws(callback=cb)
