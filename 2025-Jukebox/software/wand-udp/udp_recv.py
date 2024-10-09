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


class WandData():
    def __init__(self, address) -> None:
        self.address = address
        self.seq_num = 255
        self.plugged_in = False
        self.charged = False
        self.battery_volts = 0
        self.button = False
        self.quaternion = (0, 0, 0, 0)

    def __repr__(self) -> str:
        return (
            f"WandData(address='{self.address}', seq_num={self.seq_num}, plugged_in={self.plugged_in}, "
            f"charged={self.charged}, battery_volts={self.battery_volts}, "
            f"button={self.button}, quaternion={self.quaternion})"
        )

    def update_data(self, data) -> None:
        self.seq_num = data[0]
        self.plugged_in = data[1] == 1
        self.charged = data[2] == 1
        self.battery_volts = data[3] / 4095 * 3.7
        self.button = data[4] == 1
        self.quaternion = pyquaternion.Quaternion((data[5:9] - 16384) / 16384)


class WandServer():
    def __init__(self) -> None:
        self.udp_thread_running = False
        self.udp_thread = threading.Thread(target=self._udp_thread, daemon=True)
        self.audio_thread_running = False
        self.audio_thread = threading.Thread(target=self._audio_thread, daemon=True)
        self.stream = None
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

    def _audio_thread(self) -> None:
        while self.audio_thread_running:
            audio_buffer = []
            for addr in self.buffer:
                if not self.buffering[addr] and len(self.buffer[addr]) > 0:
                    curr_packet = self.buffer[addr].pop(0).astype(np.int32)
                    prev_packet = self.prev_audio_data[addr]
                    transition = prev_packet[-FADE_LENGTH:] * FADE_OUT + curr_packet[:FADE_LENGTH] * FADE_IN
                    audio_buffer.append(np.concatenate((transition, curr_packet[FADE_LENGTH:-FADE_LENGTH])))
                    self.prev_audio_data[addr] = curr_packet
                    if len(self.buffer[addr]) == 0:
                        self.buffering[addr] = True
            else:
                time.sleep(0.001)

            if len(audio_buffer) > 0 and self.stream is not None:
                data = [np.clip(sum(group), INT16_MIN, INT16_MAX) for group in itertools.zip_longest(*audio_buffer, fillvalue=0)]
                self.stream.write(np.array(data).astype(np.int16).tobytes())

    def start_udp(self) -> None:
        self.udp_thread_running = True
        self.udp_thread.start()

    def stop_udp(self) -> None:
        if self.udp_thread_running:
            self.udp_thread_running = False
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.connect(('127.0.0.1', PORT))
            sock.sendto(bytes([]), ('127.0.0.1', 5005))
            sock.close()
            self.udp_thread.join()

    def _udp_thread(self) -> None:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.bind(('0.0.0.0', PORT))

        while self.udp_thread_running:
            raw_data, addr = sock.recvfrom(4096)
            if len(raw_data) == 0:
                continue

            if addr not in self.wand_data:
                self.wand_data[addr] = WandData(addr)
                self.buffer[addr] = []
                self.prev_audio_data[addr] = np.zeros(FADE_LENGTH)
                self.buffering[addr] = False

            data = np.frombuffer(raw_data, np.int16)
            seq_num_diff = data[0] - self.wand_data[addr].seq_num
            if seq_num_diff > -20 and seq_num_diff <= 0:
                self.wand_data[addr].seq_num = data[0]
                continue

            self.wand_data[addr].update_data(data)
            print(repr(self.wand_data[addr]))
            if self.audio_thread_running:
                self.buffer[addr].append(data[9:])
                if len(self.buffer[addr]) > BUFFER_LIMIT and self.buffering[addr]:
                    self.buffering[addr] = False


ws = WandServer()
ws.start_udp()
ws.start_audio_output(5)
try:
    while True:
        time.sleep(0.5)
except KeyboardInterrupt:
    ws.stop_audio_output()
    ws.stop_udp()