import esp


class PinSet:
    def __init__(self):
        esp.gpio.p16_init()
        esp.gpio.p16_write(1)

    def on(self):
        esp.gpio.p16_write(0)

    def off(self):
        esp.gpio.p16_write(1)

class Indicator:
    ON = 0
    OFF = 1

    def __init__(self, pin_setter=PinSet):
        self.pin_setter = pin_setter()
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

