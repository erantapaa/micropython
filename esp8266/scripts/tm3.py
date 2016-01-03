import esp
import pyb
import gc
import network
from indicator import Indicator
from wifi_events import WifiEventManager
from srv import ControlServer

# button
# smartconfig
# server
# gpio
# json
#  (switch on 0, relay on 12 and green led on 13)


class SM:
    def wait(self):
        print("wait")

    def find_channel(self):
        print("find channel")

    def getting_ssid_pswd(self, dtype):
        print("getting ssid type", dtype)

    def link(self, ssid, password):
        print("Link", ssid, password)
        self.indicator.blink(period=500)
        network.disconnect()
        network.connect(ssid, password)

    def link_over(self, phone_ip):
        print("over done", phone_ip)
        self.indicator.indicate(self.iostate)
        esp.smartconfig.stop()

    def start(self, iostate):
        self.iostate = iostate
        self.running = True
        esp.smartconfig.start()

    def stop(self):
        self.running = False
        esp.smartconfig.stop()

    def __init__(self, indicator):
        esp.smartconfig.init(wait=lambda: self.wait,
                             find_channel=lambda: self.find_channel(),
                             getting_ssid_pswd=lambda dtype: self.getting_ssid_pswd(dtype),
                             link=lambda ssid, password: self.link(ssid, password),
                             link_over=lambda phone_ip: self.link_over(phone_ip))
        self.indicator = indicator
        self.running = False




class ButtonHandler:
    STATE_BUTTON_START = 0
    STATE_BUTTON_COLLECT = 1

    STATE_CLICK_START = 0
    STATE_CLICK_CONFIRM_CONFIG = 1
    STATE_CLICK_IN_CONFIG = 2

    def __init__(self, period=750):
        self.state = self.STATE_BUTTON_START
        self.start_time = 0
        self.period = period
        self.click_state = self.STATE_CLICK_START
        self.indicator = Indicator()
        self.smartconfig = SM(self.indicator)
        self.iostate = False

    def timer_finish(self):
        #print("Events")
        #for ii in self.events:
        #    print(ii)

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
        elif self.click_state == self.STATE_CLICK_IN_CONFIG:
            self.click_state = self.STATE_CLICK_START

        self.state = self.STATE_BUTTON_START

    def handle(self, button_value):
        if button_value == 0:
            self.iostate = not self.iostate
            self.indicator.indicate(self.iostate)

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
        cs = ControlServer(data_cb=lambda json: self.web(json))

    def add_q(self, name, qq):
        if name not in self.dqueues:
            self.dqueues[name] = qq


tm = TManager()

storage = [0 for kk in range(4)]
bq = esp.queue(storage, os_task=tm.os_task)
esp.gpio.attach(0, queue=bq, debounce=20)
tm.add_q('button', bq)

def jsh(data):
    print("got %s\n" % str(data))
    gc.collect()

