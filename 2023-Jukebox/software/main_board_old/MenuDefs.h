#ifndef MENUDEFS_H_
#define MENUDEFS_H_

void loadMainMenu();
void loadSubMenu();
void readVolPot();
void readCV1();
void readCV2();
void readCV3();
void readCV4();
void readCV5();
void readCV6();
void readCV7();
void readCV8();
void getSetAudioOption();
void getSetPreset();
void savePreset();

MenuItem mainMenu[6] = { 
  { "Audio Out 1", loadSubMenu, MENU_NAV, MENU_ITEM_DEFAULT },
  { "Audio Out 2", loadSubMenu, MENU_NAV, MENU_ITEM_DEFAULT },
  { "Audio Out 3", loadSubMenu, MENU_NAV, MENU_ITEM_DEFAULT },
  { "Audio Out 4", loadSubMenu, MENU_NAV, MENU_ITEM_DEFAULT },
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

MenuItem presetMenu[3] = {
  { "Current Preset", getSetPreset, MENU_SET, MENU_ITEM_DEFAULT },
  { "Save Peset", savePreset, MENU_SAVE, MENU_ITEM_DEFAULT },
  { "Back", loadMainMenu, MENU_NAV, MENU_ITEM_DEFAULT }
};

MenuItem debugMenu[10] = {
  { "Vol Knob", readVolPot, MENU_READ, MENU_ITEM_DEFAULT },
  { "CV1", readCV1, MENU_READ, MENU_ITEM_DEFAULT },
  { "CV2", readCV2, MENU_READ, MENU_ITEM_DEFAULT },
  { "CV3", readCV3, MENU_READ, MENU_ITEM_DEFAULT },
  { "CV4", readCV4, MENU_READ, MENU_ITEM_DEFAULT },
  { "CV5", readCV5, MENU_READ, MENU_ITEM_DEFAULT },
  { "CV6", readCV6, MENU_READ, MENU_ITEM_DEFAULT },
  { "CV7", readCV7, MENU_READ, MENU_ITEM_DEFAULT },
  { "CV8", readCV8, MENU_READ, MENU_ITEM_DEFAULT },
  { "Back", loadMainMenu, MENU_NAV, MENU_ITEM_DEFAULT }
};

MenuList listMain(mainMenu, 6);
MenuList listAudio(audioMenu, 8);
MenuList listPreset(presetMenu, 3);
MenuList listDebug(debugMenu, 10);
MenuList mainMenuList[6] = { listAudio, listAudio, listAudio, listAudio, listPreset, listDebug };

#endif /* MENUDEFS_H_ */
