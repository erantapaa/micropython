import gc
import esp
import ujson

counter = 1
last = None
lastaws = None


def cb(aws):
    global counter, last, lastaws
    counter = counter + 1
    # print("headers", aws.headers())
    if False:
        if aws.path():
            print("SRV from post URI", aws.path())
        if aws.method():
            print("SRV method", aws.method())
        if aws.status():
            print("CLI reply from get, status", aws.status())
    lastaws = aws
#    js = ujson.loads(aws.body())
#    print("%d res %s" % (counter, str(js)))
#    last = js
#    if 'test' in js and js['test'] == 1:
#        return None, "401 Bad"
#    else:
    return '{"status": %d}' % counter


def run():
    aa = esp.ws(callback=cb, local_port=80, headers=[('Content-type', 'application/json')])
    aa.listen()
    return aa
bb = "aa = run()"
aa = esp.ws(callback=cb, remote=('131.84.1.118', 8000), headers=[('Content-type', 'application/json')])
#cc = esp.ws(callback=cb, remote=('131.84.1.212', 80), headers=[('Content-type', 'application/json')])

def yy():
    global aa
    aa.async_send(body=ujson.dumps({'counter': dcounter}))

dcounter = 0

def sender(aa):
    global dcounter
    aa.async_send(body='{"counter": 1}')
    dcounter += 1
    gc.collect()

#ti = esp.os_timer(lambda timer: sender(aa), period=1000)

