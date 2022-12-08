import time
import board
import busio
#import neopixel
import math
from hx711_gpio import HX711

lfo_val = 0
lfo_rate = 10
lfo_limit = 50
lfo_dir = 1

x_min = -400 #this value is not used
x_max = -50  #this value is not used
y_min = 300  #this value is not used
y_max = 850  #this value is not used
max_voltage = 2000 #4095 is the max

#pixels = neopixel.NeoPixel(board.NEOPIXEL, 1)
i2c = busio.I2C(board.SCL, board.SDA)
x = HX711(board.A0, board.A3)
y = HX711(board.A2, board.A1)

def write_dac(device, value, dac_num):
    while not device.try_lock():
        pass
    try:
        device.writeto(0x48, bytes([0x30 + dac_num, (value & 4088) >> 4, (value & 15) << 4]))
    finally:
        device.unlock()

def clamp(value, outMin, outMax):
    return max(outMin, min(value, outMax))

def map_range(value, inMin, inMax, outMin, outMax):
    if (inMin == inMax):
        return value
    else:
        return outMin + (((value - inMin) / (inMax - inMin)) * (outMax - outMin))

def dist(x1, y1, x2, y2):
    return ((x1 - x2)**2 + (y1 - y2)**2)**0.5

def init_limits():
    global x, y, x_max, y_max, x_min, y_min
    x_val = x.read_median() / 1000
    y_val = y.read_median() / 1000
    x_min = x_val
    x_max = x_val
    y_min = y_val
    y_max = y_val

def update_limits():
    global x, y, x_max, y_max, x_min, y_min
    x_val = x.read_median() / 1000
    y_val = y.read_median() / 1000
    x_min = min(x_min, x_val)
    x_max = max(x_max, x_val)
    y_min = min(y_min, y_val)
    y_max = max(y_max, y_val)

def get_amplitudes():
    global x, y, x_max, y_max, x_min, y_min
    x_val = x.read_lowpass() / 1000
    y_val = y.read_lowpass() / 1000

    #time.sleep(0.75)
    #print(x_val, y_val)

    x_val = clamp(x_val, x_min, x_max)
    y_val = clamp(y_val, y_min, y_max)
    x_scaled = map_range(x_val, x_min, x_max, 0, max_voltage)
    y_scaled = map_range(y_val, y_min, y_max, 0, max_voltage)

    return [x_scaled, y_scaled, x_scaled, y_scaled, x_scaled, y_scaled]

def update_lfo():
    global lfo_val, lfo_rate, lfo_limit, lfo_dir
    if (lfo_dir == 1 and lfo_val > lfo_limit):
        lfo_dir = -1
    elif (lfo_dir == -1 and lfo_val < 0):
        lfo_dir = 1
    lfo_val += lfo_rate * lfo_dir


for i in range(6):
    write_dac(i2c, 0, i)
       
init_limits()
print(x_min, x_max, y_min, y_max)
start_time = time.monotonic()
while time.monotonic() - start_time < 10:
    update_limits()
x.set_limits(x_min, x_max)
y.set_limits(y_min, y_max)
print(x_min, x_max, y_min, y_max)

while True:
    update_lfo()
    vals = get_amplitudes()
    vals[0] = clamp(vals[0], 0, max_voltage)
    vals[1] = clamp(vals[1], 0, max_voltage)
    vals[2] = clamp(vals[2] + lfo_val, 0, max_voltage)
    vals[3] = clamp(vals[3] + lfo_val, 0, max_voltage)
    vals[4] = clamp(vals[4] / 4, 0, max_voltage / 4)
    vals[5] = clamp(vals[5] / 4, 0, max_voltage / 4)
    for i in range(6):
        write_dac(i2c, int(vals[i]), i)
