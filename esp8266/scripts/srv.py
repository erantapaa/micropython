import esp
import sys

def recv(socket, data):
    sys.stdout.write(data)

socket = esp.socket()
socket.onconnect(lambda s: s.onrecv(recv))
socket.bind(('0.0.0.0', 2323))
socket.listen(1)
