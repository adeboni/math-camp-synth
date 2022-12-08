#ifndef OLEDMENU_H_
#define OLEDMENU_H_

#include "Arduino.h"
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <RotaryEncoder.h>
#include <Bounce.h>

#define UP_BTN     30
#define DOWN_BTN   29
#define SELECT_BTN 28

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

#define MENU_NAV  0
#define MENU_SET  1
#define MENU_READ 2
#define MENU_SAVE 3

#define MENU_ITEM_DEFAULT  0
#define MENU_ITEM_LED_LIST 1
#define MENU_ITEM_MOD_LIST 2

typedef void (*Item_Function)();

const typedef struct MenuItem_t {
  char text[16];
  Item_Function func;
  uint8_t mode;
  uint8_t menuItemType;
} MenuItem;

class MenuList {
public:
  MenuItem *menuItems;
  uint8_t listSize;
  const char *menuTitle;
  MenuItem* getItem(int aIndex);
  MenuList(MenuItem* aList, uint8_t aSize) : menuItems(aList), listSize(aSize) {}
};

class Menu {
public:
  MenuList *currentMenu;
  bool autoUpdate = false;
  int currentSubMenu = -1;
  int currentItemIndex;
  int metaItemIndex;
  int setItemIndex = 0;
  int maxItemIndex = 0;
  int setRowSelected = -1;
  bool updatingSetting = false;
  char textBuffer[16];

  JsonArray setItemList;
  DynamicJsonDocument *templates;
  Adafruit_SH1106G *display;
  RotaryEncoder *encoder;
  Bounce *pushbutton;

  Menu();
  void init();
  void initTemplates();
  int updateSelection();
  boolean selectionMade();
  void displayMenu();
  void displayMessage(const char*);
  void doMenu();
  void updateItemList();
  void setCurrentMenu(const char*, MenuList*, bool);
  void runFunction();
  void runFunction(int);
  void getItemListText();
  const char* getText(int);
  uint8_t getMode(int);
  uint8_t getMenuItemType(int);
};

#endif /* OLEDMENU_H_ */
