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
    print("headers", aws.headers())
    if aws.path():
        print("SRV from post URI", aws.path())
    if aws.method():
        print("SRV method", aws.method())
    if aws.status():
        print("CLI reply from get, status", aws.status())
    lastaws = aws
    print("body '%s'" % aws.body())
    js = ujson.loads(aws.body())
    print("%d res %s" % (counter, str(js)))
    last = js
    return '{"status": %d}' % counter


def run():
    aa = esp.ws(callback=cb, local_port=80, headers=[('Content-type', 'application/json')])
    aa.listen()
    return aa
bb = "aa = run()"
aa = esp.ws(callback=cb, remote=('131.84.1.118', 8000), headers=[('Content-type', 'application/json')])
#cc = esp.ws(callback=cb, remote=('131.84.1.212', 80), headers=[('Content-type', 'application/json')])
