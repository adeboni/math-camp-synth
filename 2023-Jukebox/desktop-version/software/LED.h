#ifndef LED_H_
#define LED_H_

#include <OctoWS2811.h>

#define LEDS_PER_STRIP 120
#define BYTES_PER_LED  3

#define FFT_WIDTH  60
#define FFT_HEIGHT 32

#define LED_MODE_OFF     -1
#define LED_MODE_RAINBOW 0
#define LED_MODE_FFT     1

byte pinList[NUM_CHANNELS] = {2, 3, 0, 1};
DMAMEM int displayMemory[LEDS_PER_STRIP * NUM_CHANNELS * BYTES_PER_LED / 4];
int drawingMemory[LEDS_PER_STRIP * NUM_CHANNELS * BYTES_PER_LED / 4];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(LEDS_PER_STRIP, displayMemory, drawingMemory, config, NUM_CHANNELS, pinList);

class LEDChannel {
private:
    elapsedMillis lastUpdateTime;
    int rainbowColors[180];

    //FFT variables
    const float maxLevel = 0.5;      // 1.0 = max, lower is more "sensitive"
    const float dynamicRange = 40.0; // total range to display, in decibels
    const float linearBlend = 0.3;   // useful range is 0 to 0.7
    
    // This array holds the volume level (0 to 1.0) for each
    // vertical pixel to turn on.  Computed in setup() using
    // the 3 parameters above.
    float thresholdVertical[FFT_WIDTH];
    
    // This array specifies how many of the FFT frequency bin
    // to use for each horizontal pixel.  Because humans hear
    // in octaves and FFT bins are linear, the low frequencies
    // use a small number of bins, higher frequencies use more.
    int frequencyBinsHorizontal[FFT_WIDTH] = {
       1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
       2,  2,  2,  2,  2,  2,  2,  2,  2,  3,
       3,  3,  3,  3,  4,  4,  4,  4,  4,  5,
       5,  5,  6,  6,  6,  7,  7,  7,  8,  8,
       9,  9, 10, 10, 11, 12, 12, 13, 14, 15,
      15, 16, 17, 18, 19, 20, 22, 23, 24, 25
    };

    void computeVerticalLevels() {
      for (unsigned int y = 0; y < FFT_HEIGHT; y++) {
        float n = (float)y / (float)(FFT_HEIGHT - 1);
        float logLevel = pow10f(n * -1.0 * (dynamicRange / 20.0));
        float linearLevel = 1.0 - n;
        linearLevel *= linearBlend;
        logLevel *= 1.0 - linearBlend;
        thresholdVertical[y] = (logLevel + linearLevel) * maxLevel;
      }
    }
    
    int makeColor(unsigned int hue, unsigned int saturation, unsigned int lightness)
    {
      unsigned int red, green, blue;
      unsigned int var1, var2;
    
      if (hue > 359) hue = hue % 360;
      if (saturation > 100) saturation = 100;
      if (lightness > 100) lightness = 100;
    
      // algorithm from: http://www.easyrgb.com/index.php?X=MATH&H=19#text19
      if (saturation == 0) {
        red = green = blue = lightness * 255 / 100;
      } else {
        if (lightness < 50) {
          var2 = lightness * (100 + saturation);
        } else {
          var2 = ((lightness + saturation) * 100) - (saturation * lightness);
        }
        var1 = lightness * 200 - var2;
        red = h2rgb(var1, var2, (hue < 240) ? hue + 120 : hue - 240) * 255 / 600000;
        green = h2rgb(var1, var2, hue) * 255 / 600000;
        blue = h2rgb(var1, var2, (hue >= 120) ? hue - 120 : hue + 240) * 255 / 600000;
      }
      return (red << 16) | (green << 8) | blue;
    }
    
    unsigned int h2rgb(unsigned int v1, unsigned int v2, unsigned int hue)
    {
      if (hue < 60) return v1 * 60 + (v2 - v1) * hue;
      if (hue < 180) return v2 * 60;
      if (hue < 240) return v1 * 60 + (v2 - v1) * (240 - hue);
      return v1 * 60;
    }

public:
    int channelNum = -1;
    int mode = -1;
    int numLEDs = -1;
    int audioSource = -1;
    AudioAnalyzeFFT1024 *fft;

    LEDSettings() {}

    void init(int channel) {
      channelNum = channel;
      for (int i=0; i<180; i++) 
        rainbowColors[i] = makeColor(i*2, 100, 50);
      computeVerticalLevels();
    }

    void turnOff() {
      if (channelNum == -1 || numLEDs == -1) return;
      if (lastUpdateTime < 1000) return;
      
      for (int i = channelNum * LEDS_PER_STRIP; i < channelNum * LEDS_PER_STRIP + numLEDs; i++) 
        leds.setPixel(i, 0);
      leds.show();

      lastUpdateTime = 0;
    }

    void rainbow() {
      static int color = 0;
      const int phaseShift = 10;

      if (channelNum == -1 || numLEDs == -1) return;  
      if (lastUpdateTime < 2500 / numLEDs) return;

      for (int i = channelNum * LEDS_PER_STRIP; i < channelNum * LEDS_PER_STRIP + numLEDs; i++) {
        int index = (color + i + phaseShift/2) % 180;
        leds.setPixel(i, rainbowColors[index]);
      }
      leds.show();

      if (++color == 180) color = 0;
      lastUpdateTime = 0;
    }

    void FFT() {
      if (channelNum == -1 || numLEDs == -1 || audioSource == -1) return;  
      if (lastUpdateTime < 200) return;

      if (fft->available()) {
        unsigned int freqBin = 0;
        for (unsigned int x = 0; x < FFT_WIDTH; x++) {
          float level = fft->read(freqBin, freqBin + frequencyBinsHorizontal[x] - 1);
          for (unsigned int y = 0; y < FFT_HEIGHT; y++) {
            if (level >= thresholdVertical[y]) 
              leds.setPixel(channelNum * LEDS_PER_STRIP + x, rainbowColors[y * 5 % 180]);
             else 
              leds.setPixel(channelNum * LEDS_PER_STRIP + x, 0x000000);
          }
          freqBin += frequencyBinsHorizontal[x];
        }

        leds.show();
        lastUpdateTime = 0;
      }
    }

    void update() {
      if (mode == LED_MODE_OFF) turnOff();
      else if (mode == LED_MODE_RAINBOW) rainbow();
      else if (mode == LED_MODE_FFT) FFT();
    }
};

#endif /* LED_H_ */
