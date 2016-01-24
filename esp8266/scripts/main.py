import gc
import esp
import ujson

counter = 1
last = None

def cb(aws):
    global counter, last
    counter = counter + 1
#    print("got aws %r" % counter)
#    print(aws.headers())
#    print(aws.uri())
#    print(aws.method())
    js = ujson.loads(aws.body())
    print("%d res %s" % (counter, str(js)), end="\r")
    last = js
    return '{"status": %d}' % counter

aa = esp.ws(callback=cb)
aa.listen()
