import esp


class TManager:
    def handler(self, task):
        for kk, vv in self.dqueues.items():
            while True:
                try:
                    yy = vv.get()
                except Empty:
                    break
                if kk == 'temp':
                    print("temp", yy)

    def __init__(self):
        self.os_task = esp.os_task(callback=lambda tm: self.handler(tm))
        self.dqueues = dict()

    def add_q(self, name, qq):
        if name not in self.dqueues:
            self.dqueues[name] = qq


tm = TManager()
storage = [[0 for ii in range(esp.dht.DATA_SIZE)] for kk in range(4)]
q = esp.queue(storage, os_task=tm.os_task)
tm.add_q('temp', q)
aa = esp.dht(4, queue=q)
