import esp
import ujson


class ControlServer:
    def recv(self, data):
        print("date '%s'" % data)
        try:
            dd = ujson.loads(data)
        except Exception:
            print("exception on '%s'\n" % data)
        else:
            print("data '%s'\n" % str(dd))
            if 'port' in dd and 'state' in dd:
                print("port %s to state %s" % (str(dd['port']), dd['state']))

    def __init__(self, port):
        self.socket = esp.socket()
        self.socket.onconnect(lambda sock: sock.onrecv(lambda sock, data: self.recv(data)))
        self.socket.bind(('0.0.0.0', port))
        self.socket.listen(1)

cs = ControlServer(2323)
