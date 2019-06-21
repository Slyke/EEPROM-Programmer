#include <Wire.h>
#include <TM1638.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include "PCF8574.h"
#include "eeprom_io.h"
#include "http_handle.h"
#include "generic_utils.h"
#include "serial_http_prog.h"
#include "keypad_handler.h"
#include "screen_handle.h"

#define STB D7
#define CLK D6
#define DIO D5

#define SEGMENT_COUNT 8
#define HALF_SEGMENT_COUNT SEGMENT_COUNT / 2

// Internal EEPROM Addresses
#define EE_MAX_WIFI_TRIES_LOC 1
#define EE_MAX_WIFI_TRIES_LEN 1

#define EE_ENABLE_SERIAL_LOC 3
#define EE_ENABLE_SERIAL_LEN 1

#define EE_USE_NUMERIC_LOC 4
#define EE_USE_NUMERIC_LEN 1

#define EE_DISABLE_TRANSCE_LOC 6
#define EE_DISABLE_TRANSCE_LEN 1

#define EE_DISABLE_POWER_WIFI_LED_LOC 8
#define EE_DISABLE_POWER_WIFI_LED_LEN 1

#define EE_SCR_REFRESH_SPEED_LOC 10
#define EE_SCR_REFRESH_SPEED_LEN 4

#define EE_MDNS_ENABLED_LOC 16
#define EE_MDNS_ENABLED_LEN 1

#define EE_SERVER_ENABLED_LOC 17
#define EE_SERVER_ENABLED_LEN 1

#define EE_SERVER_LISTEN_PORT_LOC 18
#define EE_SERVER_LISTEN_PORT_LEN 6

#define EE_SERIAL_CON_SPD_LOC 28
#define EE_SERIAL_CON_SPD_LEN 8

#define EE_MDNS_NAME_LOC 38
#define EE_MDNS_NAME_LEN 32

#define EE_SSID_LOC 128
#define EE_SSID_LEN 64

#define EE_STAPSK_LOC 256
#define EE_STAPSK_LEN 64

#define EE_READ_GOOD_LOC 777
#define EE_READ_GOOD_LEN 1
#define EE_PROM_VERSION 0x77

// Modes
#define BASIC_ADDR_INPUT 0
#define NUMERIC_ADDR_INPUT 1
#define BASIC_MEM_CTRL 2
#define BASIC_MEM_INPUT 3
#define NUMERIC_MEM_INPUT 4
#define SERIAL_COMMAND 5  // No longer used. TODO: Remove.

#define DEFAULT_RUN_MODE BASIC_ADDR_INPUT

#define EEPROM_INTERNAL_ADDR 0x57 // I2C Address
#define EEPROM_PRIMARY_DEFAULT_ADDR 0x50 // I2C Address
#define EEPROM_SECONDARY_DEFAULT_ADDR 0x51 // I2C Address
#define EEPROM_READ_FAILURE 0xFF // Returned value on fail. Don't set to 0, 1 or EE_PROM_VERSION

#define COMM_SPEED 19200 // Default
#define REFRESH_SPEED 150 // Default
#define SERVER_LISTEN_PORT 80
#define MDNS_NAME "eeprom"

#define OP_PROCESS_ARR_LIMIT 256

byte brightness = 3;

unsigned int currentAddress = 0;
unsigned int currentMemValue = 0;
unsigned int changedValue = 0;
byte memRead = EEPROM_READ_FAILURE;
unsigned int tempMemoryEdit = 0;
bool serialEchoCommand = false;
bool assemblyInterpretMode = true;
bool serialDebugOutput = false;

unsigned char currentMode = 0;
bool modeHex = true;
char editingNumericDigitLight = -1;
bool editingNumericDigitlightState = TM1638_COLOR_NONE;

unsigned char currentScreenValue[SEGMENT_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0 };

byte scanC = 0;
byte scanR = 0;

byte loopCount = 1;

// Runtime vars read from EEPROM
String SSID = "Not set";
String PSK = "";
String mdnsName = MDNS_NAME;
byte wifiRetryTimes = 0;
byte enableSerialConn = 1;
int serialConnSpeed = 19200;
byte disableTransceiver = 0;
int refreshSpeed = REFRESH_SPEED;
byte mdnsEnabled = 1;
byte useNumeric = 1;
byte webserverEnabled = 1;
int webserverListenPort = SERVER_LISTEN_PORT;

byte primaryEepromAddress = EEPROM_PRIMARY_DEFAULT_ADDR;
byte secondaryEepromAddress = EEPROM_SECONDARY_DEFAULT_ADDR;
byte primaryEepromPointer = primaryEepromAddress;
byte secondaryEepromPointer = secondaryEepromAddress;

byte lastKeyPress = NO_KEY;

PCF8574 PCF_20(0x20);
TM1638 ioMod(DIO, CLK, STB);
ESP8266WebServer webServer(SERVER_LISTEN_PORT);

bool ICACHE_FLASH_ATTR readEepromSettings() {
  int eepromVersion = 0;
  
  exEepromReadByte((byte*)&eepromVersion, EEPROM_INTERNAL_ADDR, EE_READ_GOOD_LOC, EEPROM_READ_FAILURE);
  if (eepromVersion == EE_PROM_VERSION) {
    exEepromReadByte(&wifiRetryTimes, EEPROM_INTERNAL_ADDR, EE_MAX_WIFI_TRIES_LOC, wifiRetryTimes);
    exEepromReadByte(&enableSerialConn, EEPROM_INTERNAL_ADDR, EE_ENABLE_SERIAL_LOC, enableSerialConn);
    exEepromReadByte(&disableTransceiver, EEPROM_INTERNAL_ADDR, EE_DISABLE_TRANSCE_LOC, disableTransceiver);
    Serial.println(exEepromReadByte(&useNumeric, EEPROM_INTERNAL_ADDR, EE_USE_NUMERIC_LOC, useNumeric));

    readStringFromEeprom(&PSK, EEPROM_INTERNAL_ADDR, EE_STAPSK_LOC, EE_STAPSK_LEN);
    readStringFromEeprom(&SSID, EEPROM_INTERNAL_ADDR, EE_SSID_LOC, EE_SSID_LEN);

    String tmpSerialConnSpeed;
    readStringFromEeprom(&tmpSerialConnSpeed, EEPROM_INTERNAL_ADDR, EE_SERIAL_CON_SPD_LOC, EE_SERIAL_CON_SPD_LEN);
    serialConnSpeed = (int)strtol(tmpSerialConnSpeed.c_str(), 0, 10);

    return true;
  }

  serialConnSpeed = COMM_SPEED;
  enableSerialConn = 1;
  refreshSpeed = REFRESH_SPEED;

  return false;
}

void ICACHE_FLASH_ATTR scanI2CDevices(int devicesList[], int devicesListLenght, byte* goodDevices, int devicesErrorList[], int devicesErrorListLength, byte* badDevices) {
  byte error;
  byte address;
  *goodDevices = 0;
  *badDevices = 0;

  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      if ((int)address < devicesListLenght) {
        devicesList[*goodDevices] = address;
      }
      
      (*goodDevices)++;
    }
    else if (error == 4) {
      if ((int)address < devicesErrorListLength) {
        devicesErrorList[*badDevices] = address;
      }
      (*badDevices)++;
    }
  }
}

void breakIntToArray(unsigned char* res, int inputNumber, bool outputHex) {
  unsigned int modder = 0;
  
  if (outputHex) {
    modder = 16;
  } else {
    modder = 10;
  }

  unsigned int i = 0;
  unsigned int nCount = inputNumber;

  while (nCount != 0) {
    res[i] = nCount % modder;
    nCount /= modder;
    i++;
  }

  i = HALF_SEGMENT_COUNT - 1;
  unsigned char j = 0;
  
  while(i > j) {
    int temp = res[i];
    res[i] = res[j];
    res[j] = temp;
    i--;
    j++;
  }
}

void breakIntToArray(unsigned char* res, int inputNumber) {
  breakIntToArray(res, inputNumber, true);
}

void ICACHE_FLASH_ATTR setNewAddress(unsigned int newAddress) {
  currentAddress = newAddress & 0xffff;
}

void ICACHE_FLASH_ATTR setCurrentMode(unsigned char newMode) {
  currentMode = newMode;
  switch (newMode) {
    case BASIC_ADDR_INPUT: { // 0
      editingNumericDigitLight = -1;
      ioMod.setLED(TM1638_COLOR_RED, 0);
      ioMod.setLED(TM1638_COLOR_RED, 1);
      ioMod.setLED(TM1638_COLOR_RED, 2);
      ioMod.setLED(TM1638_COLOR_RED, 3);
      ioMod.setLED(TM1638_COLOR_NONE, 4);
      ioMod.setLED(TM1638_COLOR_NONE, 5);
      ioMod.setLED(TM1638_COLOR_NONE, 6);
      ioMod.setLED(TM1638_COLOR_NONE, 7);
      break;
    }
    case NUMERIC_ADDR_INPUT: { // 1
      ioMod.setLED(TM1638_COLOR_NONE, 0);
      ioMod.setLED(TM1638_COLOR_NONE, 1);
      ioMod.setLED(TM1638_COLOR_NONE, 2);
      ioMod.setLED(TM1638_COLOR_NONE, 3);
      ioMod.setLED(TM1638_COLOR_RED, 4);
      ioMod.setLED(TM1638_COLOR_RED, 5);
      ioMod.setLED(TM1638_COLOR_RED, 6);
      ioMod.setLED(TM1638_COLOR_RED, 7);
      editingNumericDigitLight = 3;
      editingNumericDigitlightState = TM1638_COLOR_RED;
      break;
    }
    case BASIC_MEM_CTRL: { // 2
      tempMemoryEdit = memRead;
      editingNumericDigitLight = -1;
      ioMod.setLED(TM1638_COLOR_NONE, 0);
      ioMod.setLED(TM1638_COLOR_NONE, 1);
      ioMod.setLED(TM1638_COLOR_NONE, 2);
      ioMod.setLED(TM1638_COLOR_NONE, 3);
      ioMod.setLED(TM1638_COLOR_RED, 4);
      ioMod.setLED(TM1638_COLOR_RED, 5);
      ioMod.setLED(TM1638_COLOR_RED, 6);
      ioMod.setLED(TM1638_COLOR_RED, 7);
      break;
    }
    case BASIC_MEM_INPUT: { // 3
      tempMemoryEdit = memRead;
      editingNumericDigitLight = -1;
      ioMod.setLED(TM1638_COLOR_NONE, 0);
      ioMod.setLED(TM1638_COLOR_NONE, 1);
      ioMod.setLED(TM1638_COLOR_NONE, 2);
      ioMod.setLED(TM1638_COLOR_NONE, 3);
      ioMod.setLED(TM1638_COLOR_RED, 4);
      ioMod.setLED(TM1638_COLOR_RED, 5);
      ioMod.setLED(TM1638_COLOR_RED, 6);
      ioMod.setLED(TM1638_COLOR_RED, 7);
      break;
    }
    case NUMERIC_MEM_INPUT: { // 4
      tempMemoryEdit = memRead;
      ioMod.setLED(TM1638_COLOR_NONE, 0);
      ioMod.setLED(TM1638_COLOR_NONE, 1);
      ioMod.setLED(TM1638_COLOR_NONE, 2);
      ioMod.setLED(TM1638_COLOR_NONE, 3);
//      ioMod.setLED(TM1638_COLOR_RED, 4);
//      ioMod.setLED(TM1638_COLOR_RED, 5);
//      ioMod.setLED(TM1638_COLOR_RED, 6);
//      ioMod.setLED(TM1638_COLOR_RED, 7);
      break;
    }
    case SERIAL_COMMAND: { // 5
      editingNumericDigitLight = -1;
      ioMod.setLED(TM1638_COLOR_NONE, 0);
      ioMod.setLED(TM1638_COLOR_NONE, 1);
      ioMod.setLED(TM1638_COLOR_NONE, 2);
      ioMod.setLED(TM1638_COLOR_NONE, 3);
      ioMod.setLED(TM1638_COLOR_NONE, 4);
      ioMod.setLED(TM1638_COLOR_NONE, 5);
      ioMod.setLED(TM1638_COLOR_NONE, 6);
      ioMod.setLED(TM1638_COLOR_NONE, 7);
      break;
    }
    case 6: { // 6

      break;
    }
    default: {
      editingNumericDigitLight = -1;
      ioMod.setLED(TM1638_COLOR_RED, 0);
      ioMod.setLED(TM1638_COLOR_RED, 1);
      ioMod.setLED(TM1638_COLOR_RED, 2);
      ioMod.setLED(TM1638_COLOR_RED, 3);
      ioMod.setLED(TM1638_COLOR_NONE, 4);
      ioMod.setLED(TM1638_COLOR_NONE, 5);
      ioMod.setLED(TM1638_COLOR_NONE, 6);
      ioMod.setLED(TM1638_COLOR_NONE, 7);
    }
  }
}

void ICACHE_FLASH_ATTR basicAddressInputMode() {
  if (ioMod.getButtons() == 0b00000001) {
    setNewAddress(currentAddress + 1);
  }

  if (ioMod.getButtons() == 0b00000010) {
    setNewAddress(currentAddress - 1);
  }

  if (ioMod.getButtons() == 0b00000100 || ioMod.getButtons() == 0b00001000) {
    setCurrentMode(NUMERIC_ADDR_INPUT);
  }
  
  if (ioMod.getButtons() == 0b10000000) {
    setCurrentMode(BASIC_MEM_CTRL);
  }
  
  unsigned char *addrBuffer = (unsigned char*)calloc(HALF_SEGMENT_COUNT, sizeof(char));
  breakIntToArray(addrBuffer, currentAddress, modeHex);
  updateScreen(addrBuffer, false);
  if (addrBuffer) {
    free(addrBuffer);
  }
}

void ICACHE_FLASH_ATTR numericAddressInputMode() {
  unsigned char *addrBuffer = (unsigned char*)calloc(HALF_SEGMENT_COUNT, sizeof(char));
  breakIntToArray(addrBuffer, currentAddress, modeHex);
  
  if (lastKeyPress != NO_KEY) {
    const byte editingDigit = ((HALF_SEGMENT_COUNT - 1) - editingNumericDigitLight);
    unsigned int tmpDigit = keyMap[lastKeyPress];

    setNewAddress(setDigit(currentAddress, tmpDigit, ((HALF_SEGMENT_COUNT - 1) - editingNumericDigitLight), modeHex ? 0x10 : 10));
    addrBuffer[editingNumericDigitLight] = tmpDigit;
  }

  if (ioMod.getButtons() == 0b00010000) {
    modeHex = !modeHex;
    if (modeHex) {
      updateScreen((unsigned char*)displayHex, true);
    } else {
      updateScreen((unsigned char*)displayDec, true);
    }
    ioMod.setDisplay(currentScreenValue);
    delay(1000);
    updateScreen(addrBuffer, false);
    readAndDisplayMemory();
  }
  
  if (ioMod.getButtons() == 0b00000001) {
    unsigned int tmpDigit = getDigit(currentAddress, ((HALF_SEGMENT_COUNT - 1) - editingNumericDigitLight), modeHex ? 0x10 : 10);
    if (tmpDigit == (modeHex ? 0x10 : 10)) {
      tmpDigit = 0;
    } else {
      tmpDigit++;
    }
    
    setNewAddress(setDigit(currentAddress, tmpDigit, ((HALF_SEGMENT_COUNT - 1) - editingNumericDigitLight), modeHex ? 0x10 : 10));
    addrBuffer[editingNumericDigitLight]++;
  }

  if (ioMod.getButtons() == 0b00000010) {
    unsigned int tmpDigit = getDigit(currentAddress, ((HALF_SEGMENT_COUNT - 1) - editingNumericDigitLight), modeHex ? 0x10 : 10);
    if (tmpDigit == 0) {
      tmpDigit = modeHex ? 0x10 : 10;
      tmpDigit--;
    } else {
      tmpDigit--;
    }
    setNewAddress(setDigit(currentAddress, tmpDigit, ((HALF_SEGMENT_COUNT - 1) - editingNumericDigitLight), modeHex ? 0x10 : 10));
    addrBuffer[editingNumericDigitLight]--;
  }

  if (ioMod.getButtons() == 0b00000100) {
    editingNumericDigitLight--;
    if (editingNumericDigitLight < 0) {
      editingNumericDigitLight = 3;
    }
    
    ioMod.setLED(TM1638_COLOR_RED, 0);
    ioMod.setLED(TM1638_COLOR_RED, 1);
    ioMod.setLED(TM1638_COLOR_RED, 2);
    ioMod.setLED(TM1638_COLOR_RED, 3);
  }

  if (ioMod.getButtons() == 0b00001000) {
    editingNumericDigitLight++;
    if (editingNumericDigitLight > 3) {
      editingNumericDigitLight = 0;
    }
    
    ioMod.setLED(TM1638_COLOR_RED, 0);
    ioMod.setLED(TM1638_COLOR_RED, 1);
    ioMod.setLED(TM1638_COLOR_RED, 2);
    ioMod.setLED(TM1638_COLOR_RED, 3);
  }

  if (ioMod.getButtons() == 0b10000000) {
    setCurrentMode(BASIC_ADDR_INPUT);
  }

  updateScreen(addrBuffer, false);
  if (addrBuffer) {
    free(addrBuffer);
  }
}

void ICACHE_FLASH_ATTR numericMemoryInput() {
  if (lastKeyPress != NO_KEY) {
    const byte editingDigit = HALF_SEGMENT_COUNT - (editingNumericDigitLight - (HALF_SEGMENT_COUNT - 1));
    tempMemoryEdit = setDigit(tempMemoryEdit, keyMap[lastKeyPress], editingDigit, modeHex ? 0x10 : 10) & 0xFF;
    
    unsigned char *tempMemoryEditBuffer = (unsigned char*)calloc(HALF_SEGMENT_COUNT, sizeof(char));
    breakIntToArray(tempMemoryEditBuffer, tempMemoryEdit, modeHex);

    updateScreen(tempMemoryEditBuffer, true);
    if (tempMemoryEditBuffer) {
      free(tempMemoryEditBuffer);
    }
  }

  if (ioMod.getButtons() == 0b00010000) {
    modeHex = !modeHex;
    if (modeHex) {
      updateScreen((unsigned char*)displayHex, true);
    } else {
      updateScreen((unsigned char*)displayDec, true);
    }
    ioMod.setDisplay(currentScreenValue);
    delay(1000);

    unsigned char *tempMemoryEditBuffer = (unsigned char*)calloc(HALF_SEGMENT_COUNT, sizeof(char));
    breakIntToArray(tempMemoryEditBuffer, tempMemoryEdit, modeHex);

    updateScreen(tempMemoryEditBuffer, true);
    if (tempMemoryEditBuffer) {
      free(tempMemoryEditBuffer);
    }
    readAndDisplayMemory();
  }

  if (ioMod.getButtons() == 0b00000001) {
    const byte editingDigit = HALF_SEGMENT_COUNT - (editingNumericDigitLight - (HALF_SEGMENT_COUNT - 1));
    byte tmpDigit = getDigit(tempMemoryEdit, editingDigit, modeHex ? 0x10 : 10);
    if (tmpDigit == (modeHex ? 0x10 : 10)) {
      tmpDigit = 0;
    } else {
      tmpDigit++;
    }
    tempMemoryEdit = setDigit(tempMemoryEdit, (int)tmpDigit, editingDigit, modeHex ? 0x10 : 10) & 0xFF;
    
    unsigned char *tempMemoryEditBuffer = (unsigned char*)calloc(HALF_SEGMENT_COUNT, sizeof(char));
    breakIntToArray(tempMemoryEditBuffer, tempMemoryEdit, modeHex);

    updateScreen(tempMemoryEditBuffer, true);
    if (tempMemoryEditBuffer) {
      free(tempMemoryEditBuffer);
    }
  }

  if (ioMod.getButtons() == 0b00000010) {
    const byte editingDigit = HALF_SEGMENT_COUNT - (editingNumericDigitLight - (HALF_SEGMENT_COUNT - 1));
    byte tmpDigit = getDigit(tempMemoryEdit, editingDigit, modeHex ? 0x10 : 10);
    if (tmpDigit == 0) {
      tmpDigit = modeHex ? 0x10 : 10;
      tmpDigit--;
    } else {
      tmpDigit--;
    }
    tempMemoryEdit = setDigit(tempMemoryEdit, tmpDigit, editingDigit, modeHex ? 0x10 : 10) & 0xFF;
    
    unsigned char *tempMemoryEditBuffer = (unsigned char*)calloc(HALF_SEGMENT_COUNT, sizeof(char));
    breakIntToArray(tempMemoryEditBuffer, tempMemoryEdit, modeHex);

    updateScreen(tempMemoryEditBuffer, true);
    if (tempMemoryEditBuffer) {
      free(tempMemoryEditBuffer);
    }
  }

  if (ioMod.getButtons() == 0b00000100) {
    editingNumericDigitLight--;
    if (editingNumericDigitLight < 4) {
      editingNumericDigitLight = 7;
    }
    
    ioMod.setLED(TM1638_COLOR_RED, 4);
    ioMod.setLED(TM1638_COLOR_RED, 5);
    ioMod.setLED(TM1638_COLOR_RED, 6);
    ioMod.setLED(TM1638_COLOR_RED, 7);
  }

  if (ioMod.getButtons() == 0b00001000) {
    editingNumericDigitLight++;
    if (editingNumericDigitLight > 7) {
      editingNumericDigitLight = 4;
    }
    
    ioMod.setLED(TM1638_COLOR_RED, 4);
    ioMod.setLED(TM1638_COLOR_RED, 5);
    ioMod.setLED(TM1638_COLOR_RED, 6);
    ioMod.setLED(TM1638_COLOR_RED, 7);
  }

  if (ioMod.getButtons() == 0b01000000 || ioMod.getButtons() == 0b00100000) {
    tempMemoryEdit = memRead;

    unsigned char *tempMemoryEditBuffer = (unsigned char*)calloc(HALF_SEGMENT_COUNT, sizeof(char));
    breakIntToArray(tempMemoryEditBuffer, tempMemoryEdit, modeHex);

    updateScreen(tempMemoryEditBuffer, true);
    if (tempMemoryEditBuffer) {
      free(tempMemoryEditBuffer);
    }
    setCurrentMode(BASIC_MEM_CTRL);
  }

  if (ioMod.getButtons() == 0b10000000) {
    exEepromWriteByte(primaryEepromPointer, currentAddress, tempMemoryEdit);
    setCurrentMode(BASIC_MEM_CTRL);
  }
}

void ICACHE_FLASH_ATTR basicMemoryInput() {
  if (editingNumericDigitlightState == TM1638_COLOR_RED) {
    editingNumericDigitlightState = TM1638_COLOR_NONE;
    ioMod.setLED(TM1638_COLOR_RED, 4);
    ioMod.setLED(TM1638_COLOR_RED, 5);
    ioMod.setLED(TM1638_COLOR_RED, 6);
    ioMod.setLED(TM1638_COLOR_RED, 7);
  } else {
    editingNumericDigitlightState = TM1638_COLOR_RED;
    ioMod.setLED(TM1638_COLOR_NONE, 4);
    ioMod.setLED(TM1638_COLOR_NONE, 5);
    ioMod.setLED(TM1638_COLOR_NONE, 6);
    ioMod.setLED(TM1638_COLOR_NONE, 7);
  }

  if (ioMod.getButtons() == 0b00010000) {
    modeHex = !modeHex;
    if (modeHex) {
      updateScreen((unsigned char*)displayHex, true);
    } else {
      updateScreen((unsigned char*)displayDec, true);
    }
    ioMod.setDisplay(currentScreenValue);
    delay(1000);

    unsigned char *tempMemoryEditBuffer = (unsigned char*)calloc(HALF_SEGMENT_COUNT, sizeof(char));
    breakIntToArray(tempMemoryEditBuffer, tempMemoryEdit, modeHex);

    updateScreen(tempMemoryEditBuffer, true);
    if (tempMemoryEditBuffer) {
      free(tempMemoryEditBuffer);
    }
    readAndDisplayMemory();
  }

  if (ioMod.getButtons() == 0b00000001) {
    tempMemoryEdit++;

    unsigned char *tempMemoryEditBuffer = (unsigned char*)calloc(HALF_SEGMENT_COUNT, sizeof(char));
    breakIntToArray(tempMemoryEditBuffer, tempMemoryEdit, modeHex);

    updateScreen(tempMemoryEditBuffer, true);
    if (tempMemoryEditBuffer) {
      free(tempMemoryEditBuffer);
    }
  }

  if (ioMod.getButtons() == 0b00000010) {
    tempMemoryEdit--;

    unsigned char *tempMemoryEditBuffer = (unsigned char*)calloc(HALF_SEGMENT_COUNT, sizeof(char));
    breakIntToArray(tempMemoryEditBuffer, tempMemoryEdit, modeHex);

    updateScreen(tempMemoryEditBuffer, true);
    if (tempMemoryEditBuffer) {
      free(tempMemoryEditBuffer);
    }
  }
  
  if (ioMod.getButtons() == 0b00000100 || ioMod.getButtons() == 0b00001000) {
    setCurrentMode(NUMERIC_MEM_INPUT);
    editingNumericDigitLight = 7;
  }
  
  if (ioMod.getButtons() == 0b10000000) {
    exEepromWriteByte(primaryEepromPointer, currentAddress, tempMemoryEdit);
    setCurrentMode(BASIC_MEM_CTRL);
  }
  
  if (ioMod.getButtons() == 0b01000000 || ioMod.getButtons() == 0b00100000) {
    tempMemoryEdit = memRead;

    unsigned char *tempMemoryEditBuffer = (unsigned char*)calloc(HALF_SEGMENT_COUNT, sizeof(char));
    breakIntToArray(tempMemoryEditBuffer, tempMemoryEdit, modeHex);

    updateScreen(tempMemoryEditBuffer, true);
    if (tempMemoryEditBuffer) {
      free(tempMemoryEditBuffer);
    }
    setCurrentMode(BASIC_MEM_CTRL);
  }
}

void ICACHE_FLASH_ATTR basicMemoryControl() {
  if (ioMod.getButtons() == 0b00100000 || ioMod.getButtons() == 0b01000000) {
    setCurrentMode(BASIC_MEM_INPUT);
  }

  if (ioMod.getButtons() == 0b10000000) {
    setCurrentMode(BASIC_ADDR_INPUT);
  }
}

bool ICACHE_FLASH_ATTR connectToWifi(void (*updateScr)(int)) {
  WiFi.mode(WIFI_STA);
  
  WiFi.begin(SSID.c_str(), PSK.c_str());
  int wifiTries = 0;

  Serial.print(F("Connecting To SSID: "));
  Serial.println(SSID);

  (*updateScr)(0);

  while (WiFi.status() != WL_CONNECTED && wifiTries < wifiRetryTimes) {
    wifiTries++;
    delay(500 * wifiTries);
    (*updateScr)(wifiTries);
    Serial.print(F("."));
  }

  if (wifiTries >= wifiRetryTimes) {
    Serial.println(F("\nFailed to connect to wifi."));
    Serial.print(F("Attempts tried: "));
    Serial.println(wifiTries, DEC);
    return false;
  } else {
    Serial.print(F("\nWifi connected: \n\tIP: "));
    Serial.println((String)WiFi.localIP().toString());
    Serial.print(F("\tRSSI: "));
    Serial.println((long)WiFi.RSSI());
    Serial.print(F("\tAttempts before connected: "));
    Serial.println(wifiTries, DEC);
    Serial.println(F(" "));
    return true;
  }
}

void setup() {
  Wire.begin();

  delay(10);
  bool readSettings = readEepromSettings();

  if (enableSerialConn == 1) {
    Serial.begin(serialConnSpeed);
    Serial.println(F("\nDevice booting"));
    if (!readSettings) {
      Serial.println(F("Failed to read internal EEPROM."));
    }
  }
  
  ioMod.setupDisplay(true, brightness);

  byte initialDisplay[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  for (byte i = 0, j = 7; i < 8; i++, j--) {
    initialDisplay[i] = { 255 };
    ioMod.setDisplay(initialDisplay);
    ioMod.setLED(TM1638_COLOR_RED, j);
    delay(50);
  }
  
  for (byte i = 0, j = 7; i < 8; i++, j--) {
    initialDisplay[i] = { 0 };
    ioMod.setDisplay(initialDisplay);
    ioMod.setLED(TM1638_COLOR_NONE, j);
    delay(25);
  }

  byte binDisp[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
  ioMod.setDisplay(binDisp);

  delay(10);

  byte binCycle[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
  const byte binCycleStart[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
  for (byte i = 0; i < 255; i++) {
    binCycle[0] = (binCycleStart[0] + (i * 16)) % 0xff;
    binCycle[1] = (binCycleStart[1] + (i * 16)) % 0xff;
    binCycle[2] = (binCycleStart[2] + (i * 16)) % 0xff;
    binCycle[3] = (binCycleStart[3] + (i * 16)) % 0xff;
    binCycle[4] = (binCycleStart[4] + (i * 16)) % 0xff;
    binCycle[5] = (binCycleStart[5] + (i * 16)) % 0xff;
    binCycle[6] = (binCycleStart[6] + (i * 16)) % 0xff;
    binCycle[7] = (binCycleStart[7] + (i * 16)) % 0xff;
    ioMod.setDisplay(binCycle);
    ioMod.setLED(i < 2 == 0 ? TM1638_COLOR_RED : TM1638_COLOR_NONE, 7);
    ioMod.setLED(i < 4 == 0 ? TM1638_COLOR_RED : TM1638_COLOR_NONE, 6);
    ioMod.setLED(i < 8 == 0 ? TM1638_COLOR_RED : TM1638_COLOR_NONE, 5);
    ioMod.setLED(i < 16 == 0 ? TM1638_COLOR_RED : TM1638_COLOR_NONE, 4);
    ioMod.setLED(i < 32 == 0 ? TM1638_COLOR_RED : TM1638_COLOR_NONE, 3);
    ioMod.setLED(i < 64 == 0 ? TM1638_COLOR_RED : TM1638_COLOR_NONE, 2);
    ioMod.setLED(i < 128 == 0 ? TM1638_COLOR_RED : TM1638_COLOR_NONE, 1);
    ioMod.setLED(i < 256 == 0 ? TM1638_COLOR_RED : TM1638_COLOR_NONE, 0);
    delay(1);
  }

  clearScreen(true);
  bool connectedToWifi = connectToWifi(&updateScreenConnect);

  if (connectedToWifi) {
    if (webserverEnabled == 1) {
      Serial.println(F("Starting up webserver"));
//      delete webServer;
//      webServer = new ESP8266WebServer(webserverListenPort);

      webServer.on("/", httpHandleRoot);
      webServer.on("/exec", httpHandleExec);
      webServer.on("/int", httpHandleInt);
      webServer.onNotFound(httpHandleNotFound);
      webServer.begin();
    }

    if (mdnsEnabled == 1) {
      if (MDNS.begin(mdnsName)) {
        Serial.println(F("MDNS responder started"));
      } else {
        Serial.println(F("Failed to start MDNS responder"));
      }
    }
  }

  setCurrentMode(DEFAULT_RUN_MODE);
  
  readAndDisplayAddress();
  readAndDisplayMemory();
  ioMod.setDisplay(currentScreenValue);
  
  Serial.print(F("stackAvailable: "));
//  Serial.println(stackAvailable());
  Serial.println(F("--"));
  Serial.print(F("heapAvailable: "));
  Serial.println(heapAvailable());
  
  if (Serial) {
    Serial.println(F("Device ready"));
  }
}

void loop() {
  if (editingNumericDigitLight > -1 && editingNumericDigitLight < 9) {
    if (editingNumericDigitlightState == TM1638_COLOR_RED) {
      editingNumericDigitlightState = TM1638_COLOR_NONE;
    } else {
      editingNumericDigitlightState = TM1638_COLOR_RED;
    }
    ioMod.setLED(editingNumericDigitlightState, editingNumericDigitLight);
  }

  if (WiFi.status() == WL_CONNECTED) {
    webServer.handleClient();
    MDNS.update();
  }

  delay(refreshSpeed);

  lastKeyPress = decodeKeypad(getKeyStates());
  
  if (enableSerialConn == 1) {
    if (!Serial) {
      Serial.begin(serialConnSpeed);
      if (Serial) {
        Serial.println("Serial Listening...");
      }
    }
    
    if (Serial.available()) {
      char execRes[OP_PROCESS_ARR_LIMIT] = {0};
      char echoBack[DEFAULT_MESSAGE_SIZE] = {0};
      char errorMessage[DEFAULT_MESSAGE_SIZE] = {0};
      processSerialInput(execRes, errorMessage, echoBack, OP_PROCESS_ARR_LIMIT, DEFAULT_MESSAGE_SIZE, DEFAULT_MESSAGE_SIZE);
      if (serialEchoCommand) {
        if (strlen(echoBack) > 0) {
          Serial.println(echoBack);
        }
      }
      if (strlen(execRes) > 0) {
        Serial.println(execRes);
      }
      if (strlen(errorMessage) > 0) {
        Serial.println(errorMessage);
      }
    }
  }

  switch (currentMode) {
    case BASIC_ADDR_INPUT: {
      basicAddressInputMode();
      readAndDisplayMemory();
    }
    break;
    case NUMERIC_ADDR_INPUT: {
      numericAddressInputMode();
      readAndDisplayMemory();
    }
    break;
    case BASIC_MEM_CTRL: {
      basicMemoryControl();
    }
    break;
    case BASIC_MEM_INPUT: {
      basicMemoryInput();
    }
    break;
    case NUMERIC_MEM_INPUT: {
      numericMemoryInput();
    }
    break;
    
    default: {
      setCurrentMode(DEFAULT_RUN_MODE);
    }
  }

  ioMod.setDisplay(currentScreenValue);

  if (ioMod.getButtons() == 0b00010011) {
    Reset();
  }

  if (ioMod.getButtons() == 0b11010000) {
    updateScreen((unsigned char*)displayEcho, false);
    if (serialEchoCommand) {
      updateScreen((unsigned char*)displayOff, true);
      serialEchoCommand = false;
    } else {
      updateScreen((unsigned char*)displayOn, true);
      serialEchoCommand = true;
    }
    ioMod.setDisplay(currentScreenValue);
    delay(1000);
    readAndDisplayAddress();
    readAndDisplayMemory();
  }

  loopCount = ((loopCount + 1) % 254);
}
