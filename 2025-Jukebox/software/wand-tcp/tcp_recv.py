import enum
import pyaudio
import socket
import threading
import time
import itertools
import pyquaternion
import numpy as np

INT16_MIN = np.iinfo(np.int16).min
INT16_MAX = np.iinfo(np.int16).max
FADE_LENGTH = 6
FADE_OUT = np.linspace(1, 0, FADE_LENGTH)
FADE_IN = np.linspace(0, 1, FADE_LENGTH)
BUFFER_LIMIT = 10
PORT = 5005

class AddressState(enum.Enum):
    OPEN = 1
    CLOSED = 2

class WandData():
    def __init__(self, address) -> None:
        self.address = address
        self.plugged_in = False
        self.charged = False
        self.battery_volts = 0
        self.button = False
        self.quaternion = (0, 0, 0, 0)

    def __repr__(self) -> str:
        return (
            f"WandData(address='{self.address}', plugged_in={self.plugged_in}, "
            f"charged={self.charged}, battery_volts={self.battery_volts}, "
            f"button={self.button}, quaternion={self.quaternion})"
        )
    
    def update_data(self, data) -> None:
        self.plugged_in = data[0] == 1
        self.charged = data[1] == 1
        self.battery_volts = data[2] / 4095 * 3.7
        self.button = data[3] == 1
        self.quaternion = pyquaternion.Quaternion((data[4:8] - 16384) / 16384)
        

class WandServer():
    def __init__(self) -> None:
        self.tcp_thread_running = False
        self.tcp_thread = threading.Thread(target=self._tcp_thread, daemon=True)
        self.audio_thread_running = False
        self.audio_thread = threading.Thread(target=self._audio_thread, daemon=True)
        self.stream = None
        self.addresses = {}
        self.buffer = {}
        self.prev_audio_data = {}
        self.buffering = {}
        self.wand_data = {}
        self.pa = pyaudio.PyAudio()

    def print_audio_output_devices(self) -> None:
        for i in range(self.pa.get_device_count()):
            device = self.pa.get_device_info_by_index(i)
            if device["maxOutputChannels"] > 0:
                print(i, device["name"])

    def start_audio_output(self, device_index) -> None:
        self.stream = self.pa.open(
            output=True,
            rate=16000,
            channels=1,
            format=pyaudio.paInt16,
            output_device_index=device_index,
            frames_per_buffer=1024
        )

        self.audio_thread_running = True
        self.audio_thread.start()

    def stop_audio_output(self) -> None:
        if self.audio_thread_running:
            self.audio_thread_running = False
            self.audio_thread.join()
            self.pa.close(self.stream)
            self.stream = None

    def start_tcp(self) -> None:
        self.tcp_thread_running = True
        self.tcp_thread.start()

    def stop_tcp(self) -> None:
        if self.tcp_thread_running:
            self.tcp_thread_running = False
            sock = socket.socket()
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            sock.connect(('127.0.0.1', PORT))
            sock.close()
            self.tcp_thread.join()

    def _tcp_thread(self) -> None:
        sock = socket.socket()
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        sock.bind(('0.0.0.0', PORT))
        sock.listen(5)

        while self.tcp_thread_running:
            print('Waiting for wand TCP connection...')
            conn, addr = sock.accept()
            print(f'Connected to wand at {addr}')

            if addr not in self.wand_data:
                self.wand_data[addr] = WandData(addr)
            self.buffer[addr] = []
            self.prev_audio_data[addr] = np.zeros(FADE_LENGTH)
            self.buffering[addr] = False
            self.addresses[addr] = AddressState.OPEN

            thread = threading.Thread(target=self._read_data_from_socket, args=(conn, addr), daemon=True)
            thread.start()

    def _try_parse_buffer(self, tcp_buffer, addr) -> int:
        for i in range(1, len(tcp_buffer)):
            if tcp_buffer[i] == -21846 and tcp_buffer[i-1] == -21846:
                data = tcp_buffer[:i-2]
                self.wand_data[addr].update_data(data)
                if self.audio_thread_running:
                    self.buffer[addr].append(data[8:])
                    if len(self.buffer[addr]) > BUFFER_LIMIT and self.buffering[addr]:
                        self.buffering[addr] = False
                return i + 1
        return None

    def _read_data_from_socket(self, conn, addr) -> None:
        tcp_buffer = None
        while self.tcp_thread_running:
            raw_data = conn.recv(4096)
            if raw_data == b"":
                print(f"Lost connection from wand at {addr}")
                self.addresses[addr] = AddressState.CLOSED
                break
            
            if tcp_buffer is None:
                tcp_buffer = np.frombuffer(raw_data, np.int16)
            else:
                tcp_buffer = np.concatenate((tcp_buffer, np.frombuffer(raw_data, np.int16)))
            if (start_index := self._try_parse_buffer(tcp_buffer, addr)):
                tcp_buffer = tcp_buffer[start_index:]

    def _transition_audio_data(self, x, y) -> np.array:
        transition = x[-FADE_LENGTH:] * FADE_OUT + y[:FADE_LENGTH] * FADE_IN
        return np.concatenate((transition, y[FADE_LENGTH:-FADE_LENGTH]))

    def _audio_thread(self) -> None:
        while self.audio_thread_running:
            audio_buffer = []
            for addr in [a for a in self.addresses if self.addresses[a] != AddressState.CLOSED]:
                if not self.buffering[addr] and len(self.buffer[addr]) > 0:
                    curr_packet = self.buffer[addr].pop(0).astype(np.int32)
                    prev_packet = self.prev_audio_data[addr]
                    audio_buffer.append(self._transition_audio_data(prev_packet, curr_packet))
                    self.prev_audio_data[addr] = curr_packet
                    if len(self.buffer[addr]) == 0:
                        self.buffering[addr] = True
            else:
                time.sleep(0.001)

            if len(audio_buffer) > 0 and self.stream is not None:
                data = [np.clip(sum(group), INT16_MIN, INT16_MAX) for group in itertools.zip_longest(*audio_buffer, fillvalue=0)]
                self.stream.write(np.array(data).astype(np.int16).tobytes())


ws = WandServer()
ws.start_tcp()
#ws.start_audio_output(4)
try:
    while True:
        time.sleep(0.5)
except KeyboardInterrupt:
    ws.stop_audio_output()
    ws.stop_tcp()