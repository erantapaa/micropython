import esp
import pyb


class Indicator:
    ON = 0
    OFF = 1

    def __init__(self):
        esp.gpio.p16_init()
        self.timer = None
        self.mode(self.OFF)

    def timer_target(self):
        if self.state == self.ON:
            esp.gpio.p16_write(self.OFF)
            self.state = self.OFF
        else:
            esp.gpio.p16_write(self.ON)
            self.state = self.ON

    def blink(self, period=100):
        self.timer = esp.os_timer(lambda timer: self.timer_target(), period=period)

    def mode(self, state):
        if self.timer:
            self.timer.cancel()
            self.timer = None
        esp.gpio.p16_write(state)
        self.state = state


class ButtonHandler:
    STATE_BUTTON_START = 0
    STATE_BUTTON_COLLECT = 1

    STATE_CLICK_START = 0
    STATE_CLICK_CONFIRM_CONFIG = 1
    STATE_CLICK_IN_CONFIG = 2

    def __init__(self, period=2000):
        self.state = self.STATE_BUTTON_START
        self.start_time = 0
        self.period = period
        self.click_state = self.STATE_CLICK_START
        self.indicator = Indicator()

    def timer_finish(self):
#        print("Events")
#        for ii in self.events:
#            print(ii)

        zero_count = len([ii for ii in self.events if ii[0] == 0])
        if self.click_state == self.STATE_CLICK_START:
            if zero_count == 2:
                self.indicator.blink(period=500)
                self.click_state = self.STATE_CLICK_CONFIRM_CONFIG
        elif self.click_state == self.STATE_CLICK_CONFIRM_CONFIG:
            if zero_count == 2:
                print("go into easyconfig")
                self.click_state = self.STATE_CLICK_IN_CONFIG
                self.indicator.blink()
            else:
                print("no easyconfig")
                self.click_state = self.STATE_CLICK_START
        elif self.click_state == self.STATE_CLICK_IN_CONFIG:
            print("cancel config")
            self.click_state = self.STATE_CLICK_START

        self.state = self.STATE_BUTTON_START

    def handle(self, button_value):
        if button_value == 0:
            self.indicator.mode(Indicator.ON)
        else:
            self.indicator.mode(Indicator.OFF)

        if self.state == self.STATE_BUTTON_START:
            self.start_time = pyb.millis()
            self.state = self.STATE_BUTTON_COLLECT
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
