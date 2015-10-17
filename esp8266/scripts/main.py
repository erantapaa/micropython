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


def on_connect(self, sock):
    self.soc.send(self.make_post('/', self.what))


class Socky:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.last = None
        self.soc = esp.socket()
        self.soc.onconnect(lambda sock: on_connect(self, sock))
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

    def state(self):
        return self.soc.state()

    def send(self, what):
        self.what = what
        if self.state() in [esp.ESPCONN_CLOSE, esp.ESPCONN_NONE]:
            self.soc.connect((self.host, self.port))
        else:
            self.soc.send(self.make_post('/', self.what))


def handler(task, server, dht, label):
    (hum_hi, hum_low, temp_hi, temp_low, checksum) = dht.values()
    calculated = (hum_hi + hum_low + temp_hi + temp_low) & 0xFF
    if calculated != checksum:
        print("wrong checksum %d %d" % (calculated, checksum))
        return
    temp = temp_hi * 256 + temp_low
    hum = hum_hi * 256 + hum_low
    server.send({'temp': temp, 'hum': hum, 'label': label})


def network_init(ap, password, dest_ip, dest_port):
    if esp.wifi_mode() != esp.STA_MODE:
        esp.wifi_mode(esp.STA_MODE)
    if network.status() != network.STAT_GOT_IP:
        network.connect(ap, password)
    return Socky(dest_ip, dest_port)


class sensor:
    def __init__(self, srv, name, port, period, mutex=None):
        self.srv = srv
        self.name = name
        self.port = port
        self.task = esp.os_task()
        self.dht = esp.dht(port, task=self.task, mutex=mutex)
        self.task.handler(lambda task: handler(task, self.srv, self.dht, self.name))
        self.timer = esp.os_timer(lambda timer: self.dht.recv(), period=period)


def init_temp_sensors(srv, mutex, sensor_names):
    sensors = list()
    for sensor_name, port in sensor_names.items():
        sensors.append(sensor(srv, sensor_name, port, 4000, mutex=mutex))
    return sensors

sensors_names = {'temp 1': 4,
                 'temp 2': 5}

mutex = esp.mutex()
srv = network_init('iot', 'noodlebrain', '131.84.1.191', 8000)
# sensors = init_temp_sensors(srv, mutex, sensors_names)
#tt = esp.os_task()
#aa = esp.dht(5, task=tt)
#tt.handler(lambda task: handler(task, srv, aa, 'temp 1'))

#t2 = esp.os_task()
#bb = esp.dht(4, task=t2)
#t2.handler(lambda task: handler(task, srv, bb, 'temp 2'))

#aa.recv()
#pyb.delay(100)
#bb.recv()
#pyb.delay(1000)

# esp.deepsleep(20000001)
#ta = esp.os_timer(lambda timer: aa.recv(), period=4000)
#pyb.delay(200)
#tb = esp.os_timer(lambda timer: bb.recv(), period=4000)
