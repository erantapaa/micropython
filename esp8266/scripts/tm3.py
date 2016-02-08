import esp
import pyb
import gc
import network
from wifi_events import WifiEventManager
# from srv import ControlServer
from sm import SM
from pinset import PinSet

# button
# smartconfig
# server
# gpio

class Sonoff:
    GREEN_LED = 13
    RELAY = 12



class Indicator:
    ON = 0
    OFF = 1

    def __init__(self, pin_setter):
        self.pin_setter = pin_setter
        self.timer = None
        self.state = self.OFF
        self.ons = 0

    def timer_target(self):
        if self.state == self.ON:
            self.pin_setter.off()
            self.state = self.OFF
        else:
            self.pin_setter.on()
            self.state = self.ON
            self.ons += 1

    def blink_cancel(self):
        if self.timer:
            self.timer.cancel()
            self.timer = None

    def blink(self, period=100):
        self.blink_cancel()
        self.timer = esp.os_timer(lambda timer: self.timer_target(),
                                  period=period)

    def on(self):
        self.blink_cancel()
        self.pin_setter.on()
        self.state = self.ON
        self.ons += 1

    def off(self):
        self.blink_cancel()
        self.pin_setter.off()
        self.state = self.OFF

    def indicate(self, state):
        if state:
            self.on()
        else:
            self.off()
# json
#  (switch on 0, relay on 12 and green led on 13)



class ButtonHandler:
    STATE_BUTTON_START = 0
    STATE_BUTTON_COLLECT = 1

    STATE_CLICK_START = 0
    STATE_CLICK_CONFIRM_CONFIG = 1
    STATE_CLICK_IN_CONFIG = 2

    def __init__(self, period=500):
        self.state = self.STATE_BUTTON_START
        self.start_time = 0
        self.period = period
        self.click_state = self.STATE_CLICK_START
        self.indicator = Indicator(PinSet(Sonoff.GREEN_LED))
        self.relay = PinSet(Sonoff.RELAY)
        self.smartconfig = SM(self.indicator)
        self.iostate = False

    def timer_finish(self):
#        print("Events", self.click_state)
#        for ii in self.events:
#            print(ii)

        zero_count = len([ii for ii in self.events if ii[0] == 0])
        if self.click_state == self.STATE_CLICK_START:
            if zero_count == 2:
                if self.smartconfig.running:
                    self.smartconfig.stop()
                self.indicator.blink(period=100)
                self.click_state = self.STATE_CLICK_CONFIRM_CONFIG
        elif self.click_state == self.STATE_CLICK_CONFIRM_CONFIG:
            if zero_count == 2:
                self.smartconfig.start(self.iostate)
                self.click_state = self.STATE_CLICK_IN_CONFIG
                self.indicator.blink(period=500)
            else:
                self.click_state = self.STATE_CLICK_START
                self.indicator.blink_cancel()
        elif self.click_state == self.STATE_CLICK_IN_CONFIG:
            self.click_state = self.STATE_CLICK_START

        self.state = self.STATE_BUTTON_START

    def handle(self, button_value):
        if button_value == 0:
            if self.iostate is True:
                self.iostate = False
                self.relay.off()
            else:
                self.iostate = True
                self.relay.on()

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

    def web(self, json):
        print("web %s" % str(json))

    def __init__(self):
        self.os_task = esp.os_task(callback=lambda tm: self.handler(tm))
        self.dqueues = dict()
        self.bhandler = ButtonHandler()
        self.wem = WifiEventManager()
        # cs = ControlServer(data_cb=lambda json: self.web(json))

    def add_q(self, name, qq):
        if name not in self.dqueues:
            self.dqueues[name] = qq


tm = TManager()

storage = [0 for kk in range(4)]
bq = esp.queue(storage, os_task=tm.os_task)
esp.gpio.attach(0, queue=bq, debounce=20)
tm.add_q('button', bq)


def jsh(data):
    gc.collect()

