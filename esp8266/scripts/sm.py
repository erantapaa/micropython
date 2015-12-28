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
        network.disconnect()
        network.connect(ssid, password)

    def link_over(self, phone_ip):
        print("over done", phone_ip)
        esp.smartconfig.stop()

    def __init__(self):
        esp.smartconfig.run(wait=lambda: self.wait,
                            find_channel=lambda: self.find_channel(),
                            getting_ssid_pswd=lambda dtype: self.getting_ssid_pswd(dtype),
                            link=lambda ssid, password: self.link(ssid, password),
                            link_over=lambda phone_ip: self.link_over(phone_ip))
