#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AudioFileSourceSD.h>
#include <AudioFileSourceFunction.h>
#include <AudioGeneratorWAV.h>
#include "AudioOutputI2SExtra.h"
#include <Ethernet.h>
#include <EthernetUdp.h>

#define SDA_PIN       2
#define SCL_PIN       3
#define BTN_Q7_PIN    5
#define BTN_CP_PIN    6
#define BTN_PL_PIN    7
#define SD_CLK_PIN    10
#define SD_CMD_PIN    11
#define SD_DAT3_PIN   13
#define SD_DAT2_PIN   14
#define SD_DAT1_PIN   15
#define SD_DAT0_PIN   12
#define WIZ_MISO_PIN  16
#define WIZ_CS_PIN    17
#define WIZ_SCK_PIN   18
#define WIZ_MOSI_PIN  19
#define WIZ_RST_PIN   20
#define WIZ_INT_PIN   21
#define SEG_DIG1_PIN  22
#define SEG_DIG2_PIN  9
#define SEG_SER_PIN   23
#define SEG_SCK_PIN   24
#define SEG_EN_PIN    25
#define PCM_DAT_PIN   26
#define PCM_BCK_PIN   27
#define PCM_LRCLK_PIN 28
#define VOL_PIN       29

#define JUKEBOX_MODE_MUSIC   0
#define JUKEBOX_MODE_SYNTH   1
#define JUKEBOX_MODE_EFFECTS 2
#define JUKEBOX_MODE_INVALID 255

#define PACKET_ID_ROBBIE_MODE    1
#define PACKET_ID_LASER_DATA     2
#define PACKET_ID_AUDIO_DATA     3
#define PACKET_ID_BUTTON_PRESS   4
#define PACKET_ID_AUDIO_METADATA 5
#define PACKET_ID_PLAY_EFFECT    6
#define PACKET_ID_WAND_DATA      7
#define PACKET_ID_JUKEBOX_MODE   8

#define MENU_MODE_SELECTED_SONG 0
#define MENU_MODE_CURRENT_SONG  1
#define MENU_MODE_SONG_QUEUE    2
#define MENU_MODE_JUKEBOX_MODE  3
#define MENU_MODE_WAND_DATA     4

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, -1);

const uint16_t seg_lookup[36] = {
  // pjhgfcaxdklmneb
  0b0010011101000111, //0
  0b0010001000000001, //1
  0b0100000101100011, //2
  0b0000001101100001, //3
  0b0100011000100001, //4
  0b0100011101100000, //5
  0b0100011101100010, //6
  0b0000001100000001, //7
  0b0100011101100011, //8
  0b0100011101100001, //9
  0b0100011100100011, //A
  0b0001001101101001, //B
  0b0000010101000010, //C
  0b0001001101001001, //D
  0b0100010101000010, //E
  0b0100010100000010, //F
  0b0000011101100010, //G
  0b0100011000100011, //H
  0b0001000101001000, //I
  0b0000001001000011, //J
  0b0110010000010010, //K
  0b0000010001000010, //L
  0b0010111000000011, //M
  0b0000111000010011, //N
  0b0000011101000011, //O
  0b0100010100100011, //P
  0b0000011101010011, //Q
  0b0100010100110011, //R
  0b0100011101100000, //S
  0b0001000100001000, //T
  0b0000011001000011, //U
  0b0010010000000110, //V
  0b0000011000010111, //W
  0b0010100000010100, //X
  0b0010100000001000, //Y
  0b0010000101000100  //Z
};

uint8_t currentLetter = 0;
uint8_t currentNumber = 0;
bool letterShowing = true;
uint8_t buttonStates[8] = {0, 0, 0, 0, 0, 0, 0, 0};

#define SONG_QUEUE_LIMIT 20
uint16_t songQueue[SONG_QUEUE_LIMIT];
uint16_t songQueueIndex = 0;
uint8_t songQueueLength = 0;
bool skipSong = false;

#define MAX_SONG_NAME_LEN 60
char songList[260][MAX_SONG_NAME_LEN];
uint16_t numSongs = 0;
uint16_t playingSongIndex = 0;
char effectFileName[MAX_SONG_NAME_LEN];
bool playEffect = false;
bool songPaused = false;
uint32_t songPausedPosition = 0;

uint8_t menuMode = MENU_MODE_SELECTED_SONG;
uint8_t jukeboxMode = JUKEBOX_MODE_MUSIC;
uint8_t nextJukeboxMode = JUKEBOX_MODE_INVALID;
bool displayUpdating = false;

File file;
AudioFileSourceSD *sdSource;
AudioFileSourceFunction* funcSource;
AudioGeneratorWAV *wav;
AudioOutputI2SExtra *out;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x32 };
IPAddress ip(10, 0, 0, 32);
EthernetUDP udp;
IPAddress ioIP(10, 0, 0, 31);
IPAddress laserControllerIP(10, 0, 0, 33);
#define PACKET_BUF_SIZE 1472
uint8_t packetBuffer[PACKET_BUF_SIZE];

//Packet ID, selected song ID, current song ID, num songs in queue, queued songs, current song name
uint8_t metaDataPacketBuffer[1 + 2 + 2 + 1 + SONG_QUEUE_LIMIT * 2 + MAX_SONG_NAME_LEN];
bool forceAudioData = false;

double baseVector[3] = {0.0, -1.0, 0.0};
double wandVector[3] = {0.0, 1.0, 0.0};
uint8_t wandButtonPressed = 0;

/////////////////////////////////////////////////////////////////////

void setup1() {
  Wire1.setSDA(SDA_PIN);
  Wire1.setSCL(SCL_PIN);
  Wire1.begin();

  pinMode(BTN_PL_PIN, OUTPUT);
  pinMode(BTN_Q7_PIN, INPUT);
  pinMode(BTN_CP_PIN, OUTPUT);

  pinMode(WIZ_RST_PIN, OUTPUT);
  digitalWrite(WIZ_RST_PIN, LOW);
  delay(50);
  digitalWrite(WIZ_RST_PIN, HIGH);
  delay(750);

  Ethernet.init(WIZ_CS_PIN);
  Ethernet.begin(mac, ip);
  Ethernet.setRetransmissionCount(0);
  Ethernet.setRetransmissionTimeout(0);
  udp.begin(8888);
  metaDataPacketBuffer[0] = PACKET_ID_AUDIO_METADATA;

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
}

void loop1() {
  checkButtons();
  checkForPacket();
  sendUDPAudioData();
  sendUDPAudioMetadata();
  updateDisplay();
}

void setup() {
  pinMode(VOL_PIN, INPUT);
  pinMode(SEG_DIG1_PIN, OUTPUT);
  pinMode(SEG_DIG2_PIN, OUTPUT);
  pinMode(SEG_SER_PIN, OUTPUT);
  pinMode(SEG_SCK_PIN, OUTPUT);
  pinMode(SEG_EN_PIN, OUTPUT);
  digitalWrite(SEG_DIG1_PIN, LOW);
  digitalWrite(SEG_DIG2_PIN, LOW);

  audioLogger = &Serial;
  sdSource = new AudioFileSourceSD();
  wav = new AudioGeneratorWAV();
  wav->SetBufferSize(1024);
  out = new AudioOutputI2SExtra();
  out->SetPinout(PCM_BCK_PIN, PCM_LRCLK_PIN, PCM_DAT_PIN);
  out->udpBuffer[0] = PACKET_ID_AUDIO_DATA;
  memset(out->udpBuffer + 1, 128, UDP_AUDIO_BUFF_SIZE - 1);

  SPI1.setRX(SD_DAT0_PIN);
  SPI1.setTX(SD_CMD_PIN);
  SPI1.setSCK(SD_CLK_PIN);
  SPI1.setCS(SD_DAT3_PIN);
  SD.begin(SD_DAT3_PIN, SPI_FULL_SPEED, SPI1);

  for (int i = 0; i < 260; i++)
    for (int j = 0; j < MAX_SONG_NAME_LEN; j++)
      songList[i][j] = 0;

  file = SD.open("/song_list.txt");
  if (file) {
    while (file.available() && numSongs < 260) {
      int charIndex = 0;
      while (file.available() && charIndex < MAX_SONG_NAME_LEN - 1) {
        char ch = file.read();
        if (ch == '\n' || ch == '\r' || ch == '.') break;
        songList[numSongs][charIndex++] = ch;
      }
      if (charIndex > 3)
        songList[numSongs++][charIndex] = '\0';
    }
    file.close();
  }
}

void loop() {
  updateAudio();
  updateSegDisplay();
}


/////////////////////////////////////////////////////////////////////

double mapd(double x, double in_min, double in_max, double out_min, double out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float sine_wave(const float time) {
  static double lastVal = 0.0;
  double wandYaw = atan2(wandVector[1], wandVector[0]) * 180.0 / PI;
  double pitch = mapd(wandYaw, 0.0, 360.0, 100.0, 1500.0);
  double carrier = sin(TWO_PI * pitch * time);
  double modulator = sin(TWO_PI * pitch * 0.005 * time);
  double gain = (wandVector[2] + 1.0) / 2.0 * 0.25;
  double newVal = modulator * carrier * gain;
  lastVal = 0.75 * lastVal + 0.25 * newVal;
  return (float)lastVal;
}

void sendUDPAudioData() {
  static unsigned long lastAudioDataUpdate = 0;
  static unsigned long extraDelay = 0;
  if (millis() - lastAudioDataUpdate > 50 + extraDelay && (wav->isRunning() || forceAudioData)) {
    forceAudioData = false;
    if (udp.beginPacket(laserControllerIP, 8888) == 1) {
      udp.write(out->udpBuffer, UDP_AUDIO_BUFF_SIZE);
      extraDelay = udp.endPacket() == 1 ? 0 : 1000;
    } else {
      extraDelay = 1000;
    }
    lastAudioDataUpdate = millis();
  }
}

void sendUDPAudioMetadata() {
  static unsigned long lastAudioDataUpdate = 0;
  static unsigned long extraDelay = 0;
  if (millis() - lastAudioDataUpdate > 200 + extraDelay) {
    uint16_t selectedIndex = getSelectedSongIndex();
    metaDataPacketBuffer[1] = ((uint16_t)selectedIndex >> 8) & 0xff;
    metaDataPacketBuffer[2] = selectedIndex & 0xff;
    metaDataPacketBuffer[5] = songQueueLength;
    
    for (uint8_t i = 0; i < songQueueLength; i++) {
      uint16_t songIndex = songQueue[(songQueueIndex + i) % SONG_QUEUE_LIMIT];
      metaDataPacketBuffer[6 + i * 2] = ((uint16_t)songIndex >> 8) & 0xff;
      metaDataPacketBuffer[6 + i * 2 + 1] = songIndex & 0xff;
    }
    
    if (wav->isRunning() && jukeboxMode == JUKEBOX_MODE_MUSIC) {
      for (int i = 0; i < MAX_SONG_NAME_LEN; i++)
        metaDataPacketBuffer[6 + songQueueLength * 2 + i] = songList[playingSongIndex][i];
      metaDataPacketBuffer[3] = ((uint16_t)playingSongIndex >> 8) & 0xff;
      metaDataPacketBuffer[4] = playingSongIndex & 0xff;
    } else {
      for (int i = 0; i < MAX_SONG_NAME_LEN; i++)
        metaDataPacketBuffer[6 + songQueueLength * 2 + i] = 0;
      metaDataPacketBuffer[3] = 0xff;
      metaDataPacketBuffer[4] = 0xff;
    }
  
    if (udp.beginPacket(ioIP, 8888) == 1) {
      udp.write(metaDataPacketBuffer, 6 + songQueueLength * 2 + MAX_SONG_NAME_LEN);
      extraDelay = udp.endPacket() == 1 ? 0 : 1000;
    } else {
      extraDelay = 1000;
    }

    lastAudioDataUpdate = millis();
  }
}

void checkForPacket() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    udp.read(packetBuffer, PACKET_BUF_SIZE);
    if (packetBuffer[0] == PACKET_ID_JUKEBOX_MODE && packetSize == 2) {
      nextJukeboxMode = packetBuffer[1];
    } else if (packetBuffer[0] == PACKET_ID_BUTTON_PRESS && packetSize == 2) {
      switch (packetBuffer[1]) {
        case 0:
          buttonAction(6);
          break;
        case 1:
          buttonAction(5);
          break;
        case 2:
          buttonAction(2);
          break;
        case 3:
          buttonAction(4);
          break;
        case 4:
          buttonAction(3);
          break;
        case 5:
          buttonAction(1);
          break;
        default:
          break;
      }
    } else if (packetBuffer[0] == PACKET_ID_PLAY_EFFECT && packetSize - 1 <= MAX_SONG_NAME_LEN) {
      for (int i = 1; i < packetSize; i++) {
        effectFileName[i - 1] = packetBuffer[i];
        if (packetBuffer[i] == 0) break;
      }
      playEffect = true;
    } else if (packetBuffer[0] == PACKET_ID_WAND_DATA && packetSize == 10) {
      wandButtonPressed = packetBuffer[9];

      uint16_t w = ((uint16_t)packetBuffer[1] << 8) | (uint16_t)packetBuffer[2];
      uint16_t x = ((uint16_t)packetBuffer[3] << 8) | (uint16_t)packetBuffer[4];
      uint16_t y = ((uint16_t)packetBuffer[5] << 8) | (uint16_t)packetBuffer[6];
      uint16_t z = ((uint16_t)packetBuffer[7] << 8) | (uint16_t)packetBuffer[8];
      
      double q[4] = {
        ((double)x - 16384.0) / 16384.0,
        ((double)y - 16384.0) / 16384.0,
        ((double)z - 16384.0) / 16384.0,
        ((double)w - 16384.0) / 16384.0
      };

      rotate(q, baseVector, wandVector);
    }
  }
}

void stopAudio() {
  wav->stop();
  memset(out->udpBuffer + 1, 128, UDP_AUDIO_BUFF_SIZE - 1);
  forceAudioData = true;
}

void updateAudio() {
  out->SetGain(analogRead(VOL_PIN) / 1023.0);

  if (nextJukeboxMode != JUKEBOX_MODE_INVALID) {
    if (jukeboxMode == JUKEBOX_MODE_MUSIC) {
      songPausedPosition = sdSource->getPos();
      songPaused = songPausedPosition > 0;
    }
    stopAudio();
    jukeboxMode = nextJukeboxMode;
    nextJukeboxMode = JUKEBOX_MODE_INVALID;
  }

  if (skipSong) {
    stopAudio();
    skipSong = false;
  }

  if (wav->isRunning()) {
    if (!wav->loop()) stopAudio();
    return;
  }

  if (jukeboxMode == JUKEBOX_MODE_MUSIC) {
    if (songPaused) {
      songPaused = false;
      sdSource->close();
      if (sdSource->open((String("/songs/") + String(songList[playingSongIndex]) + String(".wav")).c_str())) {
        wav->begin(sdSource, out);
        sdSource->seek(songPausedPosition, SEEK_SET);
      }
    } else {
      int nextSongIndex = dequeueSong();
      if (nextSongIndex == -1) return;
      sdSource->close();
      logSong(nextSongIndex);
      if (sdSource->open((String("/songs/") + String(songList[nextSongIndex]) + String(".wav")).c_str())) {
        wav->begin(sdSource, out);
        playingSongIndex = nextSongIndex;
      }
    }
  } else if (jukeboxMode == JUKEBOX_MODE_SYNTH) {
    funcSource = new AudioFileSourceFunction(120.0);
    funcSource->addAudioGenerators(sine_wave);
    wav->begin(funcSource, out);
  } else if (jukeboxMode == JUKEBOX_MODE_EFFECTS && playEffect) {
    playEffect = false;
    sdSource->close();
    if (sdSource->open((const char*)effectFileName))
      wav->begin(sdSource, out);
  }
}

void updateSegDisplay() {
  if (letterShowing) {
    digitalWrite(SEG_DIG1_PIN, LOW);
    digitalWrite(SEG_DIG2_PIN, LOW);
    shiftOut595(seg_lookup[currentNumber]);
    digitalWrite(SEG_DIG2_PIN, HIGH);
  } else {
    digitalWrite(SEG_DIG1_PIN, LOW);
    digitalWrite(SEG_DIG2_PIN, LOW);
    shiftOut595(seg_lookup[currentLetter + 10]);
    digitalWrite(SEG_DIG1_PIN, HIGH);
  }
  letterShowing = !letterShowing;
}

void shiftOut595(uint16_t val) {
  digitalWrite(SEG_EN_PIN, LOW);
  for (int i = 0; i < 16; i++)  {
    digitalWrite(SEG_SER_PIN, !!(val & (1 << (15 - i))));
    digitalWrite(SEG_SCK_PIN, HIGH);
    digitalWrite(SEG_SCK_PIN, LOW);
  }
  digitalWrite(SEG_EN_PIN, HIGH);
}

void buttonAction(int index) {
  switch (index) {
    case 1:
      if (jukeboxMode == JUKEBOX_MODE_MUSIC) skipSong = true;
      break;
    case 2:
      if (currentNumber > 0) currentNumber--;
      break;
    case 3:
      if (jukeboxMode == JUKEBOX_MODE_MUSIC) enqueueSong();
      break;
    case 4:
      if (currentNumber < 9 && getSongIndex(currentLetter, currentNumber + 1) < numSongs)
        currentNumber++;
      break;
    case 5:
      if (currentLetter < 25 && getSongIndex(currentLetter + 1, currentNumber) < numSongs)
        currentLetter++;
      break;
    case 6:
      if (currentLetter > 0) currentLetter--;
      break;
    case 7:
      menuMode = (menuMode + 1) % 5;
      break;
    default:
      break;
  }
  forceAudioData = true;
}

void checkButtons() {
  static unsigned long lastButtonCheck = 0;
  if (millis() - lastButtonCheck > 50) {
    digitalWrite(BTN_PL_PIN, LOW);
    digitalWrite(BTN_PL_PIN, HIGH);

    for (int i = 0; i < 8; i++) {
      uint8_t buttonState = digitalRead(BTN_Q7_PIN);
      if (buttonState && !buttonStates[i])
        buttonAction(i);
      buttonStates[i] = buttonState;
      digitalWrite(BTN_CP_PIN, HIGH);
      digitalWrite(BTN_CP_PIN, LOW);
    }
    lastButtonCheck = millis();
  }
}

void updateDisplay() {
  static unsigned long lastDisplayUpdate = 0;
  if (millis() - lastDisplayUpdate > 200) {
    display.clearDisplay();
    display.setCursor(0, 0);
  
    switch (menuMode) {
      case MENU_MODE_SELECTED_SONG:
        display.println("Selected Song: ");
        display.println(songList[getSelectedSongIndex()]);
        break;
      case MENU_MODE_CURRENT_SONG:
        display.println("Song Playing: ");
        display.print(wav->isRunning() && jukeboxMode == JUKEBOX_MODE_MUSIC ? songList[playingSongIndex] : "None");
        break;
      case MENU_MODE_SONG_QUEUE:
        display.println("Song Queue: ");
        for (uint8_t i = 0; i < songQueueLength; i++) {
          int songIndex = songQueue[(songQueueIndex + i) % SONG_QUEUE_LIMIT];
          display.print((char)((songIndex / 10) + 65));
          display.print((char)((songIndex % 10) + 48));
          display.print(" ");
        }
        break;
      case MENU_MODE_JUKEBOX_MODE:
        if      (jukeboxMode == JUKEBOX_MODE_MUSIC)   display.println("Jukebox Mode: Music");
        else if (jukeboxMode == JUKEBOX_MODE_SYNTH)   display.println("Jukebox Mode: Synth");
        else if (jukeboxMode == JUKEBOX_MODE_EFFECTS) display.println("Jukebox Mode: Effects");
        else                                          display.println("Jukebox Mode: Unknown");
        break;
      case MENU_MODE_WAND_DATA:
        char buf[10];
        display.println("Wand Data:");
        dtostrf(wandVector[0], 5, 2, buf);
        display.print(buf);
        dtostrf(wandVector[1], 5, 2, buf);
        display.print(buf);
        dtostrf(wandVector[2], 5, 2, buf);
        display.print(buf);
        display.print(" ");
        display.print(wandButtonPressed);
      default:
        break;
    }
  
    display.display();
    lastDisplayUpdate = millis();
  }
}

uint16_t getSelectedSongIndex() {
  return getSongIndex(currentLetter, currentNumber);
}

uint16_t getSongIndex(uint8_t letter, uint8_t number) {
  return 10 * (uint16_t)letter + number;
}

void enqueueSong() {
  if (songQueueLength == SONG_QUEUE_LIMIT)
    return;
  if (songQueueLength > 0 && songQueue[(songQueueIndex + songQueueLength - 1) % SONG_QUEUE_LIMIT] == getSelectedSongIndex())
    return;
  songQueue[(songQueueIndex + songQueueLength) % SONG_QUEUE_LIMIT] = getSelectedSongIndex();
  songQueueLength++;
}

int dequeueSong() {
  if (songQueueLength == 0)
    return -1;
  uint16_t songIndex = songQueue[songQueueIndex];
  songQueueLength--;
  songQueueIndex = (songQueueIndex + 1) % SONG_QUEUE_LIMIT;
  return songIndex;
}

void logSong(int index) {
  File f = SD.open("play_log.txt", FILE_WRITE);
  if (f) {
    f.println(songList[index]);
    f.close();
  }
}

void rotate(double q[4], double v[3], double result[3]) {
  double conj[4] = { -q[0], -q[1], -q[2], q[3] };
  double qv[4] = {
    q[3] * v[0] + q[1] * v[2] - q[2] * v[1],
    q[3] * v[1] + q[2] * v[0] - q[0] * v[2],
    q[3] * v[2] + q[0] * v[1] - q[1] * v[0],
    -q[0] * v[0] - q[1] * v[1] - q[2] * v[2]
  };
  result[0] = qv[3] * conj[0] + qv[0] * conj[3] + qv[1] * conj[2] - qv[2] * conj[1];
  result[1] = qv[3] * conj[1] + qv[1] * conj[3] + qv[2] * conj[0] - qv[0] * conj[2];
  result[2] = qv[3] * conj[2] + qv[2] * conj[3] + qv[0] * conj[1] - qv[1] * conj[0];
}
