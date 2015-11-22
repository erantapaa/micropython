# This script is run on boot
import esp, network, re
import ujson
import pyb


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


class Socky:
    def __init__(self, host, port):
        self.what = dict()
        self.host = host
        self.port = port
        self.last = None
        self.soc = esp.socket()
        self.soc.onconnect(lambda sock: send_data(self))
        self.soc.onrecv(lambda sock, data: socket_recv(self, sock, data))

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

    def runner(self):
        pass

    def run(self, period):
        pass

    def state(self):
        return self.soc.state()

    def send(self, what):
        self.what.update(what)
        if self.state() in [esp.ESPCONN_CLOSE, esp.ESPCONN_NONE]:
            self.soc.connect((self.host, self.port))
        else:
            send_data(self)


def network_init(ap, password, dest_ip, dest_port):
    if esp.wifi_mode() != esp.STA_MODE:
        esp.wifi_mode(esp.STA_MODE)
    if network.status() != network.STAT_GOT_IP:
        network.connect(ap, password)
    return Socky(dest_ip, dest_port)


class Sensor:
    def __init__(self, name, port, period, task=None, mutex=None):
        self.name = name
        self.dht = esp.dht(port, task=task, mutex=mutex, spinwait=True)
        self.timer = esp.os_timer(lambda timer: self.dht.recv(), period=period)

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

    def __init__(self):
        self.i2c = esp.I2C()
        self.i2c.write([self.AUTO], self.BH1750_address)

    def read(self):
        values = self.i2c.read(2, self.BH1750_address)
        return values[1] + (values[0] << 8)


def handler(task, server, sensors_manager):
    sensors = dict()
    for name, sensor in sensors_manager.sensors.items():
        try:
            temp, hum = sensor.get()
            sensors['temp ' + name] = temp
            sensors['hum ' + name] =  hum
        except Exception:
            continue
    sensors['light 1'] = sensors_manager.light_sensor.read()
    server.send(sensors)


class SensorsManager:
    def __init__(self, sensor_table, srv):
        self.light_sensor = LightSensor()
        self.mutex = esp.mutex()
        self.sensor_table = sensor_table
        self.sensors = dict()
        self.task = esp.os_task(callback=lambda task: handler(task, srv, self))
        for sensor_name, sensor_port in sensor_table.items():
            self.sensors[sensor_name] = Sensor(sensor_name, sensor_port, 10000, task=self.task, mutex=self.mutex)
            pyb.udelay(10000)


sensors_names = {'1': 5}

srv = network_init('iot', 'noodlebrain', '131.84.1.191', 8000)
mgr = SensorsManager(sensors_names, srv)
