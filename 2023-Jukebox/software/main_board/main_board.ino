#define NUM_CHANNELS 4

#include "StringUtils.h"
#include "AudioChain.h"
#include "OledMenu.h"
#include "MenuDefs.h"
#include "Globals.h"
#include <EEPROM.h>

void readInputs() { 
  for (int i = 0; i < 8; i++)
    cv[i] = map(analogRead(cvPins[i]), 0, 1023, 0, 100);

  volKnob = map(analogRead(27), 0, 1023, 0, 100);
  sgtl5000_1.dacVolume((float)volKnob / 100);
  sgtl5000_2.dacVolume((float)volKnob / 100);
}

void updateAudioSettings(int channel, const char *option, const char *newVal) {
  AudioNoInterrupts();
  if (strcmp(option, "Source") == 0) {
     audioChain[channel].wavPlaying = false;
     audioChain[channel].playSdWav.stop();
     delete audioChain[channel].vcoToVcf;
    
     if (strcmp(newVal, "EXT IN") == 0) {
       audioChain[channel].vcoToVcf = new AudioConnection(audioInput, channel, audioChain[channel].filter, 0);
       audioChain[channel].wavPlaying = false;
     } else {
       sprintf(audioChain[channel].wavFileName, "/Audio/%s.raw", newVal);
       audioChain[channel].playSdWav.play(audioChain[channel].wavFileName);
       audioChain[channel].vcoToVcf = new AudioConnection(audioChain[channel].playSdWav, 0, audioChain[channel].filter, 0);
       delay(10);
       audioChain[channel].wavPlaying = true;
     }
     
  } else if (strcmp(option, "Filter Frq") == 0) {
     audioChain[channel].vcfFrqMod = audioChain[channel].getKnobValue(newVal);
     if (audioChain[channel].vcfFrqMod == -1)
        audioChain[channel].setVcfFrq(percentStrToInt(newVal));
        
  } else if (strcmp(option, "Filter Res") == 0) {
     audioChain[channel].vcfResMod = audioChain[channel].getKnobValue(newVal);
     if (audioChain[channel].vcfResMod == -1)
        audioChain[channel].setVcfRes(percentStrToInt(newVal));
        
  } else if (strcmp(option, "Effect") == 0) {
     delete audioChain[channel].vcfToEffect;
     delete audioChain[channel].effectToVca;
    
    if (strcmp(newVal, "None") == 0) {
       audioChain[channel].selectedEffect = -1;
       audioChain[channel].vcfToEffect = new AudioConnection(audioChain[channel].filter, 0, audioChain[channel].amp, 0);
    } else if (strcmp(newVal, "Reverb") == 0) {
       audioChain[channel].selectedEffect = 0;
       audioChain[channel].vcfToEffect = new AudioConnection(audioChain[channel].filter, 0, audioChain[channel].reverb, 0);
       audioChain[channel].effectToVca = new AudioConnection(audioChain[channel].reverb, 0, audioChain[channel].amp, 0);
    } else if (strcmp(newVal, "Granular") == 0) {
       audioChain[channel].selectedEffect = 1;
       audioChain[channel].vcfToEffect = new AudioConnection(audioChain[channel].filter, 0, audioChain[channel].granular, 0);
       audioChain[channel].effectToVca = new AudioConnection(audioChain[channel].granular, 0, audioChain[channel].amp, 0);
    } 
    
  } else if (strcmp(option, "Effect Mod") == 0) {
    audioChain[channel].effectMod = audioChain[channel].getKnobValue(newVal);
     if (audioChain[channel].effectMod == -1)
        audioChain[channel].setEffectMod(percentStrToInt(newVal));
        
  } else if (strcmp(option, "Gain Mod") == 0) {
    audioChain[channel].vcaMod = audioChain[channel].getKnobValue(newVal);
     if (audioChain[channel].vcaMod == -1)
        audioChain[channel].setVcaMod(percentStrToInt(newVal));
  }

  AudioInterrupts();
}

void updateAudioSettings() {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    updateAudioSettings(i, "Source", preset["Audio"][i]["Source"].as<char*>());
    updateAudioSettings(i, "Source Mod", preset["Audio"][i]["Source Mod"].as<char*>());
    updateAudioSettings(i, "Filter Frq", preset["Audio"][i]["Filter Frq"].as<char*>());
    updateAudioSettings(i, "Filter Res", preset["Audio"][i]["Filter Res"].as<char*>());
    updateAudioSettings(i, "Effect", preset["Audio"][i]["Effect"].as<char*>());
    updateAudioSettings(i, "Effect Mod", preset["Audio"][i]["Effect Mod"].as<char*>());
    updateAudioSettings(i, "Gain Mod", preset["Audio"][i]["Gain Mod"].as<char*>());

    delete audioChain[i].vcaToOutput;
    audioChain[i].vcaToOutput = new AudioConnection(audioChain[i].amp, 0, audioOutput, i);
  }
}

void stopWavFiles() {
  for (int i = 0; i < NUM_CHANNELS; i++)
    audioChain[i].playSdWav.stop();
}

void resumeWavFiles() {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    if (audioChain[i].wavPlaying && !audioChain[i].playSdWav.isPlaying())
      audioChain[i].playSdWav.play(audioChain[i].wavFileName);
  }
}

void savePreset() {
  stopWavFiles();
  int presetNum = 0;
  File presetsRoot = SD.open("/Presets/");
  while (true) {
     File entry = presetsRoot.openNextFile();
     if (!entry) break;
     if (strEndsWith(entry.name(), ".txt")) {
        char buf[16];
        trimExtension(entry.name(), buf);
        presetNum = max(atoi(buf), presetNum);
     }
     entry.close();
  }
  presetsRoot.close();

  char filename[20];
  sprintf(filename, "/Presets/%d.txt", ++presetNum);
  SD.remove(filename);

  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println(F("Failed to create file"));
    return;
  }

  serializeJson(preset, file);
  file.close();

  currPreset = presetNum;
  EEPROM.write(0, currPreset);
}

void loadPreset(int presetNum) {
  stopWavFiles();
  Serial.print("Loading preset ");
  Serial.println(presetNum);
  
  char filename[20];
  sprintf(filename, "/Presets/%d.txt", presetNum);

  File file = SD.open(filename);
  if (!file) {
    if (presetNum > 1) {
      loadPreset(presetNum - 1);
      return;
    }
    Serial.println(F("Failed to open file"));
    return;
  }

  deserializeJson(preset, file);
  file.close();

  currPreset = presetNum;
  EEPROM.write(0, currPreset);
  updateAudioSettings();
}

void setup() {
  Serial.begin(9600);
  AudioMemory(40);

  AudioNoInterrupts();
  sgtl5000_1.setAddress(HIGH);
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
  sgtl5000_1.adcHighPassFilterEnable();
  sgtl5000_1.lineInLevel(5);
  sgtl5000_1.dacVolumeRamp();
  sgtl5000_1.dacVolume(0);

  sgtl5000_2.setAddress(LOW);
  sgtl5000_2.enable();
  sgtl5000_2.inputSelect(AUDIO_INPUT_LINEIN);
  sgtl5000_2.adcHighPassFilterEnable();
  sgtl5000_2.lineInLevel(5);
  sgtl5000_2.dacVolumeRamp();
  sgtl5000_2.dacVolume(0);
  AudioInterrupts();

  menu.init();
  menu.setCurrentMenu("Main Menu", &listMain, false);
  loadPreset(EEPROM.read(0));
}

void loop() {
  menu.doMenu();
  readInputs();
  resumeWavFiles();
}

void getSetAudioOption() {
  int i = menu.currentSubMenu;
  int j = menu.metaItemIndex;

  if (menu.updatingSetting) {
     menu.getItemListText();
     preset["Audio"][i][audioMenu[j].text] = menu.textBuffer;
     currPreset = -1;
     updateAudioSettings(i, audioMenu[j].text, menu.textBuffer);
  } else {
     strcpy(menu.textBuffer, preset["Audio"][i][audioMenu[j].text].as<char*>());
  }
}

void getSetPreset() {
  if (menu.updatingSetting) {
     menu.getItemListText();
     if (strcmp(menu.textBuffer, "None") == 0)
        return;
     currPreset = atoi(menu.textBuffer);
     loadPreset(currPreset);
  } else {
     if (currPreset == -1)
       strcpy(menu.textBuffer, "None");
     else
       sprintf(menu.textBuffer, "%d", currPreset);
  }
}


void loadMainMenu() {
  menu.currentSubMenu = -1;
  menu.setCurrentMenu("Main Menu", &listMain, false);
}

void loadSubMenu() {
  menu.currentSubMenu = menu.currentItemIndex;
  menu.setCurrentMenu(mainMenu[menu.currentItemIndex].text, 
    &mainMenuList[menu.currentItemIndex], 
    menu.currentItemIndex == 5);
}

void readVolPot() { sprintf(menu.textBuffer, "%d", volKnob); }
void readCV1() { sprintf(menu.textBuffer, "%d", cv[0]); }
void readCV2() { sprintf(menu.textBuffer, "%d", cv[1]); }
void readCV3() { sprintf(menu.textBuffer, "%d", cv[2]); }
void readCV4() { sprintf(menu.textBuffer, "%d", cv[3]); }
void readCV5() { sprintf(menu.textBuffer, "%d", cv[4]); }
void readCV6() { sprintf(menu.textBuffer, "%d", cv[5]); }
void readCV7() { sprintf(menu.textBuffer, "%d", cv[6]); }
void readCV8() { sprintf(menu.textBuffer, "%d", cv[7]); }
