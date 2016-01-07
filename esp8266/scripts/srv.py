import esp
import ujson, re


class ControlServer:
    def make_response(self, data=None, status="200 OK"):
        if data is None:
            data = {"result": "OK"}
        status_line = 'HTTP/1.1 200 OK\r\n'
        headers = {'Content-Type': 'application/json',
                   'Content-Length': 0}
        data_as_text = ujson.dumps(data)
        headers['Content-Length'] = len(data_as_text)
        to_send = status_line + '\r\n'.join("%s: %s" % (field, value) for field, value in headers.items())
        to_send += '\r\n\r\n'
        to_send += data_as_text
        return to_send

    def recv(self, sock, data):
        for ii in data.split(b'\r\n'):
            if ii.startswith(b'POST / HTTP/1.'):
                self.state = 'header'
            elif self.state == 'header':
                if ii == b'':
                    self.state = 'body'
                    continue
                hdr = re.search('^(.*): *(.*)$', ii)
                if not hdr:
                    print("did not match '%s'" % ii)

            elif self.state == 'body':
                if ii == b'':
                    continue
                try:
                    if self.data_cb:
                        sock.send(self.make_response(self.data_cb(ujson.loads(ii))))
                    else:
                        print("GOT '%s'" % str(ujson.loads(ii)))
                        sock.send(self.make_response())
                except Exception as ee:
                    sock.send(self.make_response(data={'error': str(ee)}, status='400 Bad Request'))
                sock.close()

    def onconnect(self, sock):
        self.state = 'action'
        sock.onrecv(lambda sock, data: self.recv(sock, data))

    def __init__(self, port=80, data_cb=None):
        self.data_cb = data_cb
        self.socket = esp.socket()
        self.socket.onconnect(lambda sock: self.onconnect(sock))
        self.socket.bind(('0.0.0.0', port))
        self.socket.listen(1)


def jsh(data):
    print("got %s\n" % str(data))

cs = ControlServer(data_cb=jsh)
