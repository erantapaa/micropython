import esp


class Indicator:
    ON = 0
    OFF = 1

    def __init__(self, pin_setter):
        self.pin_setter = pin_setter()
        self.timer = None

    def timer_target(self):
        if self.state == self.ON:
            self.pin_setter.off()
            self.state = self.OFF
        else:
            self.pin_setter.on()
            self.state = self.ON

    def blink(self, period=100):
        if self.timer:
            self.timer.cancel()
            self.timer = None
        self.timer = esp.os_timer(lambda timer: self.timer_target(),
                                  period=period)

    def mode(self, state):
        if self.timer:
            self.timer.cancel()
            self.timer = None
        if state == self.ON:
            self.pin_setter.on()
        else:
            self.pin_setter.off()
        self.state = state


class PinSet:
    def __init__(self):
        esp.gpio.p16_init()
        esp.gpio.p16_write(1)

    def on(self):
        esp.gpio.p16_write(0)

    def off(self):
        esp.gpio.p16_write(1)
