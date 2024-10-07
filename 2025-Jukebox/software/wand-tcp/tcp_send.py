import socket
import time
import numpy as np

AUDIO_SAMPLES = 320
SIN_SCALING = 0.2

sock = socket.socket()
sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
sock.connect(('127.0.0.1', 5005))

def get_packet():
    data = np.array([np.int16(x) for x in range(8)]).tobytes()
    for i in range(AUDIO_SAMPLES):
        data += np.array([np.int16(10000 * np.sin(i * SIN_SCALING))]).tobytes()
    data += np.array([np.int16(-21846), np.int16(-21846)]).tobytes()
    return data

start_time = time.time()
while time.time() - start_time < 5:
    sock.send(get_packet())

sock.close()
