import board
import digitalio
import time

class HX711:
    def __init__(self, sck, out):
        self.pOUT = digitalio.DigitalInOut(out)
        self.pSCK = digitalio.DigitalInOut(sck)
        self.pSCK.direction = digitalio.Direction.OUTPUT
        self.pSCK.value = False

        self.MIN_VAL = 0
        self.MAX_VAL = 0
        self.LIMITS_SET = 0
        self.PREV_VALUE = 0

        self.GAIN = 1
        self.OFFSET = 0

        self.time_constant = 0.25
        self.filtered = 0        
        self.read()
        self.filtered = self.read()

    def read(self):
        while self.pOUT.value == 1:
            time.sleep(0.001)

        result = 0
        for j in range(24 + self.GAIN):
            self.pSCK.value = True
            self.pSCK.value = False
            result = (result << 1) | self.pOUT.value

        result >>= self.GAIN
        if result > 0x7fffff:
            result -= 0x1000000

        #print(result)

        value = result - self.OFFSET
        if (self.LIMITS_SET == 1 and abs(value - self.PREV_VALUE) > (self.MAX_VAL * 1000 - self.MIN_VAL * 1000) * 0.1):
            value = self.PREV_VALUE
        else:
            self.PREV_VALUE = value

        return value

    def set_limits(self, min_val, max_val):
        self.MIN_VAL = min_val
        self.MAX_VAL = max_val
        self.LIMITS_SET = 1

    def read_average(self, times=3):
        sum = 0
        for i in range(times):
            sum += self.read()
        return sum / times

    def read_lowpass(self):
        self.filtered += self.time_constant * (self.read() - self.filtered)
        return self.filtered
        
    def read_median(self):
        x1 = self.read_lowpass()
        x2 = self.read_lowpass()
        x3 = self.read_lowpass()
        
        if (x1 > x2 and x1 < x3):
            return x1
        elif (x2 > x1 and x2 < x3):
            return x2
        else:
            return x3

    def tare(self, times=80):
        self.OFFSET = 0
        self.OFFSET = self.read_average(times)

    def power_down(self):
        self.pSCK.value = False
        self.pSCK.value = True

    def power_up(self):
        self.pSCK.value = False
