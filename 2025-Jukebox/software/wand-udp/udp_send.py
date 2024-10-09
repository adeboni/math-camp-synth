import socket
import time
import pyquaternion
import numpy as np

AUDIO_HZ = 16000
AUDIO_SAMPLES = 512
TIME_PER_PACKET = AUDIO_SAMPLES / AUDIO_HZ * 0.95
SIN_SCALING = 0.2

def quaternion_generator():
    q1 = pyquaternion.Quaternion.random()
    q2 = pyquaternion.Quaternion.random()
    while True:
        for q in pyquaternion.Quaternion.intermediates(q1, q2, 20, include_endpoints=True):
            yield q
        q1 = q2
        q2 = pyquaternion.Quaternion.random()

def packet_generator():
    gen = quaternion_generator()
    packet_num = 0
    while True:
        data = np.array([np.int16(x) for x in [packet_num, 0, 1, 0, 0, 0]]).tobytes()
        packet_num = (packet_num + 1) & 0xFF
        quat = next(gen)
        data += np.array([np.int16(quat[i] * 16384 + 16384) for i in range(4)]).tobytes()
        for i in range(AUDIO_SAMPLES):
            data += np.array([np.int16(10000 * np.sin(i * SIN_SCALING))]).tobytes()
        data += np.array([np.int16(-21846), np.int16(-21846)]).tobytes()
        yield data


gen = packet_generator()
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

try:
    start_time = time.time()
    while True:
        sock.sendto(next(gen), ('127.0.0.1', 5005))
        time.sleep(TIME_PER_PACKET)
except KeyboardInterrupt:
    sock.close()
