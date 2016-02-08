import esp
import network


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
        print("blink to disconnect")
        network.disconnect()
        print("disconnected to connect")
        network.connect(ssid, password)
        print("connected")

    def link_over(self, phone_ip):
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
