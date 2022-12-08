#include "OledMenu.h"
#include "StringUtils.h"

void Menu::initTemplates() {
  templates = new DynamicJsonDocument(2048);
  
  int i = 0;
  (*templates)["Audio"]["Source"][i++] = "EXT IN";

  File audioRoot = SD.open("/Audio/");
  while (true) {
     File entry = audioRoot.openNextFile();
     if (!entry) break;
     if (strEndsWith(entry.name(), ".raw")) {
        char buf[16];
        trimExtension(entry.name(), buf);
        (*templates)["Audio"]["Source"][i++] = buf;
     }
     entry.close();
  }
  audioRoot.close();

  const int NUM_AUDIO_MODS = 5;
  const char *AUDIO_MOD_LIST[] = {"Source Mod", "Filter Frq", "Filter Res", "Effect Mod", "Gain Mod"};
  for (int j = 0; j < NUM_AUDIO_MODS; j++) {
    i = 0;
    (*templates)["Audio"][AUDIO_MOD_LIST[j]][i++] = "CV1";
    (*templates)["Audio"][AUDIO_MOD_LIST[j]][i++] = "CV2";
    (*templates)["Audio"][AUDIO_MOD_LIST[j]][i++] = "CV3";
    (*templates)["Audio"][AUDIO_MOD_LIST[j]][i++] = "CV4";
    (*templates)["Audio"][AUDIO_MOD_LIST[j]][i++] = "CV5";
    (*templates)["Audio"][AUDIO_MOD_LIST[j]][i++] = "CV6";
    (*templates)["Audio"][AUDIO_MOD_LIST[j]][i++] = "CV7";
    (*templates)["Audio"][AUDIO_MOD_LIST[j]][i++] = "CV8";
  }

  (*templates)["Audio"]["Effect"][0] = "None";
  (*templates)["Audio"]["Effect"][1] = "Reverb";
  (*templates)["Audio"]["Effect"][2] = "Granular";
  i = 0;
  (*templates)["Presets"]["Current Preset"][i++] = "None";
  File presetsRoot = SD.open("/Presets/");
  while (true) {
     File entry = presetsRoot.openNextFile();
     if (!entry) break;
     if (strEndsWith(entry.name(), ".txt")) {
        char buf[16];
        trimExtension(entry.name(), buf);
        (*templates)["Presets"]["Current Preset"][i++] = buf;
     }
     entry.close();
  }
  presetsRoot.close();

  //serializeJsonPretty((*templates), Serial);
}

MenuItem* MenuList::getItem(int aIndex) {
  return &(menuItems[aIndex]);
}

Menu::Menu() {
  currentMenu = 0;
  currentItemIndex = 0;
}

void Menu::runFunction() {
  Item_Function theFunc = currentMenu->getItem(currentItemIndex)->func;
  theFunc();
}

void Menu::runFunction(int aIndex) {
  metaItemIndex = aIndex;
  Item_Function theFunc = currentMenu->getItem(aIndex)->func;
  theFunc();
}

void Menu::updateItemList() {
  setRowSelected = currentItemIndex;
  setItemIndex = 0;
  
  if (currentSubMenu < 4) {
    setItemList = (*templates)["Audio"][getText(setRowSelected)];
  } else if (currentSubMenu < 9) {
    setItemList = (*templates)["Presets"][getText(setRowSelected)];
  }

  maxItemIndex = setItemList.size();
  uint8_t menuItemType = getMenuItemType(setRowSelected);
  if (menuItemType == MENU_ITEM_MOD_LIST)
    maxItemIndex += 101;
  else if (menuItemType == MENU_ITEM_LED_LIST)
    maxItemIndex += 121;

  runFunction(setRowSelected);
  for (int i = 0; i < maxItemIndex; i++) {
    char buf[20];
    if (i < setItemList.size())
      strcpy(buf, setItemList[i].as<char*>());
    else if (menuItemType == MENU_ITEM_MOD_LIST)
      sprintf(buf, "%d%%", i - setItemList.size());
    else if (menuItemType == MENU_ITEM_LED_LIST)
      sprintf(buf, "%d", i);
    
    if (strcmp(textBuffer, buf) == 0) {
      setItemIndex = i;
      break;
    }
  }
}

void Menu::doMenu() {
  if (updateSelection() == 0 && selectionMade()) {
     if (getMode(currentItemIndex) == MENU_NAV) {
        runFunction();
     } else if (getMode(currentItemIndex) == MENU_SAVE) {
        runFunction();
        displayMenu();
     } else if (getMode(currentItemIndex) == MENU_SET && setRowSelected == -1) {
        updateItemList();
        displayMenu();
     } else if (getMode(currentItemIndex) == MENU_SET && setRowSelected >= 0) {
        setRowSelected = -1;
        displayMenu();
     }
  }

  static unsigned long prevTime = millis();
  if (autoUpdate) {
    unsigned long newTime = millis();
    if (newTime - prevTime > 100) {
      displayMenu();
      prevTime = newTime;
    }
  }
}

uint8_t Menu::getMode(int aIndex) {
  return currentMenu->getItem(aIndex)->mode;
}

const char* Menu::getText(int aIndex) {
  return currentMenu->getItem(aIndex)->text;
}

uint8_t Menu::getMenuItemType(int aIndex) {
  return currentMenu->getItem(aIndex)->menuItemType;
}

void Menu::setCurrentMenu(const char* title, MenuList* aMenu, bool refresh) {
  currentMenu = aMenu;
  currentMenu->menuTitle = title;
  currentItemIndex = 0;
  autoUpdate = refresh;
  displayMenu();
}

void Menu::init() {
  display = new Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
  encoder = new RotaryEncoder(UP_BTN, DOWN_BTN, RotaryEncoder::LatchMode::TWO03);
  pushbutton = new Bounce(SELECT_BTN, 10);
  
  pinMode(SELECT_BTN, INPUT_PULLUP);
  display->begin(0x3C, true);
  display->clearDisplay();
  display->setTextSize(1);
  display->setTextColor(SH110X_WHITE);
  display->display();

  if (!SD.begin(BUILTIN_SDCARD)) {
    displayMessage("SD init fail");
    while (true) ;
  }

  initTemplates();
}

int Menu::updateSelection() {
  int i = 0;
  static int pos = 0;
  encoder->tick();
  int newPos = encoder->getPosition();
  if (pos != newPos) {
    i = (int)(encoder->getDirection());
    pos = newPos;
  }
 
  if (i != 0) {
    if (setRowSelected >= 0) {
      setItemIndex += i;
      setItemIndex = max(0, setItemIndex);
      setItemIndex = min(maxItemIndex - 1, setItemIndex);

      updatingSetting = true;
      runFunction(setRowSelected);
      updatingSetting = false;
    } else {
      currentItemIndex += i;
      currentItemIndex = max(0, currentItemIndex);
      currentItemIndex = min(currentMenu->listSize - 1, currentItemIndex);
    }
    displayMenu();
  }
  return i;
}

boolean Menu::selectionMade() {
  return pushbutton->update() && pushbutton->fallingEdge();
}

void Menu::displayMessage(const char* msg) {
  display->clearDisplay();
  display->setCursor(0, 0);
  display->print(msg);
  display->display();
}

void Menu::getItemListText() {
  uint8_t menuItemType = getMenuItemType(setRowSelected);
  if (setItemIndex < setItemList.size())
    strcpy(textBuffer, setItemList[setItemIndex].as<char*>());
  else if (menuItemType == MENU_ITEM_MOD_LIST)
    sprintf(textBuffer, "%d%%", setItemIndex - setItemList.size());
  else if (menuItemType == MENU_ITEM_LED_LIST)
    sprintf(textBuffer, "%d", setItemIndex);
}

void Menu::displayMenu() {
  display->clearDisplay();
  display->setCursor(0, 0);
  display->print(currentMenu->menuTitle);

  int startPos = 0;
  if (currentItemIndex > 4) 
    startPos = currentItemIndex - 4;
  
  for (int i = startPos; i < min(startPos + 6, currentMenu->listSize); i++) { // for each menu item
    display->setCursor(0, (i - startPos + 1) * 10);
    display->print(i == currentItemIndex ? '>' : ' ');
    display->print(getText(i));

    if (getMode(i) == MENU_READ) {
       display->print(": ");
       runFunction(i);
       display->print(textBuffer);
    }
    else if (getMode(i) == MENU_SET) {
       display->print(": ");
       if (setRowSelected == i) {
         display->setTextColor(SH110X_BLACK, SH110X_WHITE);
         getItemListText();
         display->print(textBuffer);
       } else {
         runFunction(i);
         display->print(textBuffer);
       }
       display->setTextColor(SH110X_WHITE);
    }
  }
  display->display();
}
