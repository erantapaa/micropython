# This script is run on boot
import esp, network, re

network.connect('iot', 'noodlebrain')


def socket_recv(self, sock, data):
    if data == b'HTTP/1.0 200 OK\r\n':
        return
    state = 'header'
    for ii in data.split(b'\r\n'):
        if state == 'header':
            if ii == b'':
                state = 'body'
                continue
            hdr = re.search('^([A-Za-z0-9-]+): *(.*)$', ii)
            if hdr and hdr.group(1) == 'Date':
                self.mutex.release()
                sock.close()
                self.got_date(hdr.group(2))
                return
        else:
            self.mutex.release()
            sock.close()


def on_connect(self, sock):
    method = 'GET / HTTP/1.1\r\n'
    headers = {'Content-Type': 'application/json',
               'Content-Length': 0}
    to_send = method + '\r\n'.join("%s: %s" % (field, value) for field, value in headers.items())
    to_send += '\r\n\r\n'
    self.soc.send(to_send)


class Socky:
    def __init__(self, got_date):
        self.soc = esp.socket()
        self.soc.onconnect(lambda sock: on_connect(self, sock))
        self.soc.onrecv(lambda sock, data: socket_recv(self, sock, data))
        self.mutex = esp.mutex(spin_time=3000)
        self.got_date = got_date

    def get_date(self, host, port):
        try:
            self.mutex.acquire()
        except Exception:
            print("Already running too long")
            return
        self.soc.connect((host, port))


class Ds3231:
    DS3231_DEVICE_ADDR = 0x68
    DS3231_ADDR_TIME = 0x00

    def __init__(self, i2c):
        self.i2c = i2c

    @staticmethod
    def http_time_to_ds_array(aa):
        md = {'Jan': 1, 'Feb': 2, 'Mar': 3, 'Apr': 4, 'May': 5, 'Jun': 6, 'Jul': 7, 'Aug': 8, 'Sep': 9, 'Oct': 10, 'Nov': 11, 'Dec': 12}
        wdays = {'Sun': 1, 'Mon': 2, 'Tue': 3, 'Wed': 4, 'Thu': 5, 'Fri': 6, 'Sat': 7}
        bb = re.match('(.*), ([0-9]+) (...) ([0-9]+) ([0-2][0-9]):([0-5][0-9]):([0-5][0-9])', aa)
        data = [int(bb.group(7)),   # Second
                int(bb.group(6)),  # Minute
                int(bb.group(5)),  # Hour
                wdays[bb.group(1)],  # Weekday number
                int(bb.group(2)),  # Day
                md[bb.group(3)],   # Month
                int(bb.group(4)) - 1900]  # YY
        return data

    @staticmethod
    def decToBcd(dec):
        return ((dec // 10) * 16) + (dec % 10)

    @staticmethod
    def bcdToDec(bcd):
        return ((bcd // 16) * 10) + (bcd % 16)

    @staticmethod
    def format_da(aa):
        return "%02d/%02d/%02d %02d:%02d:%02d" % (aa[6] + 1900, aa[5], aa[4], aa[2], aa[1], aa[0])

    def set_date(self, date):
        print("date '%s'" % date)
        st = [self.DS3231_ADDR_TIME] + list(map(self.decToBcd, self.http_time_to_ds_array(date)))
        self.i2c.write(st, self.DS3231_DEVICE_ADDR)

    def date(self):
        i2c.write([self.DS3231_ADDR_TIME], self.DS3231_DEVICE_ADDR)
        return self.format_da(list(map(self.bcdToDec, i2c.read(7, self.DS3231_DEVICE_ADDR))))


i2c = esp.I2C()
ds = Ds3231(i2c)
srv = Socky(lambda date: ds.set_date(date))


def got_address(name, tt):
    srv.get_date(tt[-1][0], tt[-1][1])


def set_date():
    esp.getaddrinfo('www.google.com', 80, got_address)


def pp(timer):
    print("\033[2J")
    print(ds.date())

