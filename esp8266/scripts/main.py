# This script is run on boot
import esp, network, re
import ujson


class Socky:
    def __init__(self, host, port):
        self.what = dict()
        self.host = host
        self.port = port
        self.last = None
        self.soc = esp.socket()
        self.soc.onconnect(lambda sock: self.send_data())
        self.soc.ondisconnect(lambda sock: self.finished())
        self.soc.onrecv(lambda sock, data: self.socket_recv(sock, data))

    def socket_recv(self, sock, data):
        if data == b'HTTP/1.0 200 OK\r\n':
            return
        state = 'header'
        for ii in data.split(b'\r\n'):
            if state == 'header':
                if ii == b'':
                    state = 'body'
                    continue
                hdr = re.search('^(.*): *(.*)$', ii)
                if not hdr:
                    print("did not match '%s'" % ii)

            else:
                self.last = ujson.loads(ii)
                sock.close()

    # todo how to sent multiple blocks when we need to wait?
    def send_data(self):
        if self.what:
            self.soc.send(self.make_post('/esp_probe', self.what))
            self.what = dict()

    def finished(self):
        print("fnished")
        esp.deepsleep(30 * 1000000)

    def make_post(self, url, data):
        method = 'POST %s HTTP/1.1\r\n' % url
        headers = {'Content-Type': 'application/json',
                   'Content-Length': 0}
        data_as_text = ujson.dumps(data)
        headers['Content-Length'] = len(data_as_text)
        to_send = method + '\r\n'.join("%s: %s" % (field, value) for field, value in headers.items())
        to_send += '\r\n\r\n'
        to_send += data_as_text
        return to_send

    def send(self, what):
        self.what.update(what)
        if self.soc.state() in [esp.ESPCONN_CLOSE, esp.ESPCONN_NONE]:
            self.soc.connect((self.host, self.port))
        else:
            self.send_data(self)


def network_init(ap, password, dest_ip, dest_port):
    if esp.wifi_mode() != esp.STA_MODE:
        esp.wifi_mode(esp.STA_MODE)
    if network.status() != network.STAT_GOT_IP:
        network.connect(ap, password)
    return Socky(dest_ip, dest_port)


class Sensor:
    def __init__(self, name, port, task=None, mutex=None):
        self.name = name
        self.dht = esp.dht(port, task=task, mutex=mutex, spinwait=True)
        # self.timer = esp.os_timer(lambda timer: self.dht.recv(), period=period)

    def recv(self):
        self.dht.recv()

    @staticmethod
    def sensor_tuple_to_temp_hum(stuple):
        (hum_hi, hum_low, temp_hi, temp_low, checksum) = stuple
        calculated = (hum_hi + hum_low + temp_hi + temp_low) & 0xFF
        if calculated != checksum:
            raise ValueError("wrong checksum %d %d" % (calculated, checksum))
        # TODO: extra logic for negative (never going to happen in Sydney)
        return temp_hi * 256 + temp_low, hum_hi * 256 + hum_low

    def get(self):
        return self.sensor_tuple_to_temp_hum(self.dht.values(consume=True))


class LightSensor:
    BH1750_address = 0x23
    AUTO = 0x10

    def __init__(self, name):
        self.name = name
        self.i2c = esp.I2C()
        self.i2c.write([self.AUTO], self.BH1750_address)

    def recv(self):
        values = self.i2c.read(2, self.BH1750_address)
        return values[1] + (values[0] << 8)


class SensorsManager:
    def __init__(self, sensor_table, srv):
        self.light_sensor = LightSensor('light 1')
        self.send_on = set([self.light_sensor.name])
        self.mutex = esp.mutex()
        self.sensor_table = sensor_table
        self.sensors = dict()
        self.to_send = dict()
        self.task = esp.os_task(callback=lambda task: self.handler(task, srv))
        for sensor_name, sensor_port in sensor_table.items():
            self.sensors[sensor_name] = Sensor(sensor_name, sensor_port, task=self.task, mutex=self.mutex)
            self.send_on.add('temp ' + sensor_name)
            self.send_on.add('hum ' + sensor_name)
            # pyb.udelay(10000) TODO: make sure two dht can be used at once

    def handler(self, task, server):
        for name, sensor in self.sensors.items():
            try:
                temp, hum = sensor.get()
                self.to_send['temp ' + name] = temp
                self.to_send['hum ' + name] = hum
            except Exception:
                continue
        self.to_send[self.light_sensor.name] = self.light_sensor.recv()
        if set(self.to_send.keys()) == self.send_on:
            server.send(self.to_send)

    def receive(self):
        for sensor in self.sensors.values():
            sensor.recv()


srv = network_init('iot', 'noodlebrain', '131.84.1.191', 8000)

sensors_names = {'1': 5}
mgr = SensorsManager(sensors_names, srv)

t = esp.os_timer(lambda tt: mgr.receive(), period=1000, repeat=True)
