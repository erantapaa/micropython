# This script is run on boot
import esp


class TMan:
    def handler(self, task):
        print("inside hander")
        for kk, vv in self.actions.items():
            while True:
                try:
                    yy = vv.get()
                except StopIteration:
                    break
                print("from", kk, "val", yy)

    def __init__(self):
        self.os_task = esp.os_task(callback=lambda tm: self.handler(tm))
        self.actions = dict()

    def add_tq(self, name, value):
        if name not in self.actions:
            self.actions[name] = esp.queue(5)
        self.actions[name].put(value)

aa = TMan()
aa.add_tq('aaakk', 123)
# aa.os_task.post()

