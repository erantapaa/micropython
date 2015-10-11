# This script is run on boot
import esp


def handler(task, dht, name):
    (hum_hi, hum_low, temp_hi, temp_low, checksum) = dht.values()
    calculated = (hum_hi + hum_low + temp_hi + temp_low) & 0xFF
    if calculated != checksum:
        print("wrong checksum %d %d" % (calculated, checksum))
        return
    temp = temp_hi * 256 + temp_low
    hum = hum_hi * 256 + hum_low
    print("%s temp %d.%d hum %d.%d" % (name, temp // 10, temp % 10, hum // 10, hum % 10))


tt = esp.os_task()
aa = esp.dht(5, task=tt)
tt.handler(lambda task: handler(task, aa, 'temp 1'))

t2 = esp.os_task()
bb = esp.dht(4, task=t2)
t2.handler(lambda task: handler(task, bb, 'temp 2'))

ta = esp.os_timer(lambda bb: aa.recv(), period=4000)
tb = esp.os_timer(lambda bb: bb.recv(), period=4000)
