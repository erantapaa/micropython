import esp
import network


class WifiEventManager:
    def stamode_connected(self, ssid, channel):
        print("Connected to %s channel %d" % (ssid, channel))
    def stamode_disconnected(self, ssid, reason):
        print("Disconnected from '%s' reason %d" % (ssid, reason))
    def stamode_got_ip(self, ip, mask, gw):
        print("Got ip %s mask %s from gw %s" % (ip, mask, gw))
    def __init__(self, ap_ssid, ap_password):
        esp.wifi_events.init(stamode_connected=lambda ssid, channel: self.stamode_connected(ssid, channel), stamode_disconnected=lambda ssid, reason: self.stamode_disconnected(ssid, reason), stamode_got_ip=lambda ip, mask, gw: self.stamode_got_ip(ip, mask, gw))
        network.connect(ap_ssid, ap_password)

