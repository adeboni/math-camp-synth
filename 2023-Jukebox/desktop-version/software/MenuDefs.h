#ifndef MENUDEFS_H_
#define MENUDEFS_H_

void loadMainMenu();
void loadSubMenu();
void readVolPot();
void readPitchPot();
void readFilterPot();
void readGainPot();
void readXVal();
void readYVal();
void readAngleVal();
void readRadiusVal();
void readMIDIVal();
void getSetAudioOption();
void getSetLEDOption();
void getSetPreset();
void savePreset();

MenuItem mainMenu[10] = { 
  { "Audio Out 1", loadSubMenu, MENU_NAV, MENU_ITEM_DEFAULT },
  { "Audio Out 2", loadSubMenu, MENU_NAV, MENU_ITEM_DEFAULT },
  { "Audio Out 3", loadSubMenu, MENU_NAV, MENU_ITEM_DEFAULT },
  { "Audio Out 4", loadSubMenu, MENU_NAV, MENU_ITEM_DEFAULT },
  { "LED Out 1", loadSubMenu, MENU_NAV, MENU_ITEM_DEFAULT },
  { "LED Out 2", loadSubMenu, MENU_NAV, MENU_ITEM_DEFAULT },
  { "LED Out 3", loadSubMenu, MENU_NAV, MENU_ITEM_DEFAULT },
  { "LED Out 4", loadSubMenu, MENU_NAV, MENU_ITEM_DEFAULT },
  { "Presets", loadSubMenu, MENU_NAV, MENU_ITEM_DEFAULT },
  { "Debug", loadSubMenu, MENU_NAV, MENU_ITEM_DEFAULT }
};

MenuItem audioMenu[8] = {
  { "Source", getSetAudioOption, MENU_SET, MENU_ITEM_DEFAULT },
  { "Source Mod", getSetAudioOption, MENU_SET, MENU_ITEM_MOD_LIST },
  { "Filter Frq", getSetAudioOption, MENU_SET, MENU_ITEM_MOD_LIST },
  { "Filter Res", getSetAudioOption, MENU_SET, MENU_ITEM_MOD_LIST },
  { "Effect", getSetAudioOption, MENU_SET, MENU_ITEM_DEFAULT },
  { "Effect Mod", getSetAudioOption, MENU_SET, MENU_ITEM_MOD_LIST },
  { "Gain Mod", getSetAudioOption, MENU_SET, MENU_ITEM_MOD_LIST },
  { "Back", loadMainMenu, MENU_NAV, MENU_ITEM_DEFAULT }
};

MenuItem ledMenu[4] = {
  { "Mode", getSetLEDOption, MENU_SET, MENU_ITEM_DEFAULT },
  { "Audio Source", getSetLEDOption, MENU_SET, MENU_ITEM_DEFAULT },
  { "Num LEDs", getSetLEDOption, MENU_SET, MENU_ITEM_LED_LIST },
  { "Back", loadMainMenu, MENU_NAV, MENU_ITEM_DEFAULT }
};

MenuItem presetMenu[3] = {
  { "Current Preset", getSetPreset, MENU_SET, MENU_ITEM_DEFAULT },
  { "Save Peset", savePreset, MENU_SAVE, MENU_ITEM_DEFAULT },
  { "Back", loadMainMenu, MENU_NAV, MENU_ITEM_DEFAULT }
};

MenuItem debugMenu[10] = {
  { "VCO Knob", readPitchPot, MENU_READ, MENU_ITEM_DEFAULT },
  { "VCF Knob", readFilterPot, MENU_READ, MENU_ITEM_DEFAULT },
  { "VCA Knob", readGainPot, MENU_READ, MENU_ITEM_DEFAULT },
  { "Vol Knob", readVolPot, MENU_READ, MENU_ITEM_DEFAULT },
  { "X", readXVal, MENU_READ, MENU_ITEM_DEFAULT },
  { "Y", readYVal, MENU_READ, MENU_ITEM_DEFAULT },
  { "Angle", readAngleVal, MENU_READ, MENU_ITEM_DEFAULT },
  { "Radius", readRadiusVal, MENU_READ, MENU_ITEM_DEFAULT },
  { "MIDI", readMIDIVal, MENU_READ, MENU_ITEM_DEFAULT },
  { "Back", loadMainMenu, MENU_NAV, MENU_ITEM_DEFAULT }
};

MenuList listMain(mainMenu, 10);
MenuList listAudio(audioMenu, 8);
MenuList listLed(ledMenu, 4);
MenuList listPreset(presetMenu, 3);
MenuList listDebug(debugMenu, 10);

MenuList mainMenuList[10] = {
  listAudio, listAudio, listAudio, listAudio,
  listLed, listLed, listLed, listLed,
  listPreset, listDebug
};

#endif /* MENUDEFS_H_ */
