import esp

class TManager:
    def handler(self, task):
        for kk, vv in self.actions.items():
            while True:
                try:
                    yy = vv.get()
                except Empty:
                    break
                print("from", kk, "val", yy)

    def __init__(self):
        self.os_task = esp.os_task(callback=lambda tm: self.handler(tm))
        self.actions = dict()

    def add_tq(self, name, value, post=False):
        if name not in self.actions:
            self.actions[name] = esp.queue(5)
        self.actions[name].put(value)
        if post:
            self.os_task.post()
