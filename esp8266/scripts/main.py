import gc
import esp
import ujson

counter = 1
last = None
lastaws = None

def cb(aws):
    global counter, last, lastaws
    counter = counter + 1
    print("got aws %r" % counter)
    print(aws.headers())
    print(aws.uri())
    print(aws.method())
    lastaws = aws
    print("body '%s'" % aws.body())
    js = ujson.loads(aws.body())
    print("%d res %s" % (counter, str(js)))
    last = js
    return '{"status": %d}' % counter

# aa = "aa = esp.ws(callback=cb)"
#aa.listen()
aa = esp.ws(callback=cb, remote=('131.84.1.118', 8000))
