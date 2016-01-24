import gc
import esp
def cb(aws):
    print("got aws")
    print(aws.headers())
    print(aws.uri())
    print(aws.method())

aa = esp.ws(callback=cb)
