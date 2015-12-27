import esp
import pyb


class ButtonHandler:
    START = 0
    COLLECT = 1

    def __init__(self, period=2000):
        self.state = self.START
        self.start_time = 0
        self.period = period
        esp.gpio.p16_init()
        esp.gpio.p16_write(1)

    def timer_finish(self):
        print("Events")
        for ii in self.events:
            print(ii)
        print("0s", len([ii for ii in self.events if ii[0] == 0]))
        self.state = self.START

    def handle(self, button_value):
        if button_value == 0:
            esp.gpio.p16_write(0)
        else:
            esp.gpio.p16_write(1)

        if self.state == self.START:
            self.start_time = pyb.millis()
            self.state = self.COLLECT
            self.events = list()
            self.timer = esp.os_timer(lambda timer: self.timer_finish(), period=self.period, repeat=False)
            elapsed = 0
        else:
            elapsed = pyb.elapsed_millis(self.start_time)
        self.events.append((button_value, elapsed))


class TManager:
    def handler(self, task):
        for kk, vv in self.dqueues.items():
            while True:
                try:
                    yy = vv.get()
                except Empty:
                    break
                if kk == 'button':
                    self.bhandler.handle(yy)
                else:
                    print("kk %s val %s" % (kk, str(yy)))

    def __init__(self):
        self.os_task = esp.os_task(callback=lambda tm: self.handler(tm))
        self.dqueues = dict()
        self.bhandler = ButtonHandler()

    def add_q(self, name, qq):
        if name not in self.dqueues:
            self.dqueues[name] = qq


tm = TManager()
storage = [[0 for ii in range(esp.dht.DATA_SIZE)] for kk in range(4)]
q = esp.queue(storage, os_task=tm.os_task)
tm.add_q('temp', q)
aa = esp.dht(4, queue=q)

storage = [0 for kk in range(4)]
bq = esp.queue(storage, os_task=tm.os_task)
esp.gpio.attach(0, queue=bq, debounce=20)
tm.add_q('button', bq)
