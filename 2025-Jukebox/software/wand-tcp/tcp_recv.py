import enum
import pyaudio
import socket
import threading
import time
import itertools
import numpy as np

INT16_MIN = np.iinfo(np.int16).min
INT16_MAX = np.iinfo(np.int16).max
BUFFER_LIMIT = 10 # TODO: check latency with real microphone
PORT = 5005

class AddressState(enum.Enum):
    OPEN = 1
    CLOSING = 2
    CLOSED = 3

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
        self.quaternion = (data[4], data[5], data[6], data[7])

class WandServer():
    def __init__(self) -> None:
        self.tcp_thread_running = False
        self.audio_thread_running = False
        self.stream = None
        self.addresses = {}
        self.buffer = {}
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
        self.audio_thread = threading.Thread(target=self._audio_thread, daemon=True)
        self.audio_thread.start()

    def stop_audio_output(self) -> None:
        self.audio_thread_running = False
        self.audio_thread.join()
        self.pa.close(self.stream)
        self.stream = None

    def start_tcp(self) -> None:
        self.tcp_thread_running = True
        self.tcp_thread = threading.Thread(target=self._tcp_thread, daemon=True)
        self.tcp_thread.start()

    def stop_tcp(self) -> None:
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
            self.buffering[addr] = False
            self.addresses[addr] = AddressState.OPEN

            thread = threading.Thread(target=self._read_data_from_socket, args=(conn, addr), daemon=True)
            thread.start()

    def _read_data_from_socket(self, conn, addr) -> None:
        while self.tcp_thread_running:
            raw_data = conn.recv(4096)
            if raw_data == b"":
                print(f"Lost connection from wand at {addr}")
                self.addresses[addr] = AddressState.CLOSING
                break

            data = np.frombuffer(raw_data, np.int16)
            self.wand_data[addr].update_data(data)
            #print(repr(self.wand_data[addr]))
            self.buffer[addr].append(data[8:])
            if len(self.buffer[addr]) > BUFFER_LIMIT and self.buffering[addr]:
                self.buffering[addr] = False

    def _audio_thread(self) -> None:
        while self.audio_thread_running:
            audio_buffer = []
            for addr in [a for a in self.addresses if self.addresses[a] != AddressState.CLOSED]:
                if not self.buffering[addr] and len(self.buffer[addr]) > 0:
                    audio_buffer.append(self.buffer[addr].pop(0).astype(np.int32))
                    if len(self.buffer[addr]) == 0:
                        self.buffering[addr] = True
                elif self.addresses[addr] == AddressState.CLOSING:
                    self.addresses[addr] = AddressState.CLOSED
            else:
                time.sleep(0.001)

            if len(audio_buffer) > 0 and self.stream is not None:
                data = [np.clip(sum(group), INT16_MIN, INT16_MAX) for group in itertools.zip_longest(*audio_buffer, fillvalue=0)]
                self.stream.write(np.array(data).astype(np.int16).tobytes())


ws = WandServer()
ws.start_tcp()
ws.start_audio_output(5)
try:
    while True:
        time.sleep(0.5)
except KeyboardInterrupt:
    ws.stop_audio_output()
    ws.stop_tcp()