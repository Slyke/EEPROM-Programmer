#include <Wire.h>
#include <TM1638.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>

// #define EZ_MODE

#define STB D7
#define CLK D6
#define DIO D5

#define NO_KEY 0x32

#define SEGMENT_COUNT 8
#define HALF_SEGMENT_COUNT SEGMENT_COUNT / 2

// Internal EEPROM Addresses
#define EE_MAX_WIFI_TRIES_LOC 1
#define EE_MAX_WIFI_TRIES_LEN 1

#define EE_ENABLE_SERIAL_LOC 3
#define EE_ENABLE_SERIAL_LEN 1

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

#define OP_PROCESS_ARR_LIMIT 512

// OP Codes
#define OP_NOP 0x00
#define OP_JMP 0x01
#define OP_AIC 0x02
#define OP_ADC 0x03
#define OP_GETC 0x04
#define OP_GET 0x05
#define OP_MOV 0x06
#define OP_L2M 0x07
#define OP_PI2CC 0xd1
#define OP_SI2CC 0xd2
#define OP_PI2C 0xd3
#define OP_SI2C 0xd4
#define OP_PERC 0xd6
#define OP_PER 0xd7
#define OP_SERC 0xd8
#define OP_SER 0xd9
#define OP_PCPY 0xda
#define OP_INT 0xe0
#define OP_ECH 0xe1
#define OP_ADMP 0xe2
#define OP_CDMP 0xe3
#define OP_RETD 0xf0
#define OP_RET 0xf1
#define OP_I2C 0xf4
#define OP_RST 0xf9
#define STR_OP_NOP "0x00"
#define STR_OP_JMP "0x01"
#define STR_OP_AIC "0x02"
#define STR_OP_ADC "0x03"
#define STR_OP_GETC "0x04"
#define STR_OP_GET "0x05"
#define STR_OP_MOV "0x06"
#define STR_OP_L2M "0x07"
#define STR_OP_PI2CC "0xd1"
#define STR_OP_SI2CC "0xd2"
#define STR_OP_PI2C "0xd3"
#define STR_OP_SI2C "0xd4"
#define STR_OP_PERC "0xd6"
#define STR_OP_PER "0xd7"
#define STR_OP_SERC "0xd8"
#define STR_OP_SER "0xd9"
#define STR_OP_PCPY "0xda"
#define STR_OP_INT "0xe0"
#define STR_OP_ECH "0xe1"
#define STR_OP_ADMP "0xe2"
#define STR_OP_CDMP "0xe3"
#define STR_OP_RETD "0xf0"
#define STR_OP_RET "0xf1"
#define STR_OP_I2C "0xf4"
#define STR_OP_RST "0xf9"

const byte pinRows = 4;
const byte pinCols = 4;
const byte outputPins[pinRows] = {13, 12, 11, 10};
const byte inputPins[pinCols] = {9, 8, 7, 6};
const unsigned char displayError[4] = {46, 38, 40, 38};
const unsigned char displayDec[4] = {16, 13, 14, 12};
const unsigned char displayHex[4] = {16, 42, 14, 44};
const unsigned char displayEcho[4] = {46, 12, 42, 0};
const unsigned char displayOn[4] = {16, 16, 0, 1};
const unsigned char displayOff[4] = {16, 0, 15, 15};
const unsigned char NoRes[4] = {18, 18, 18, 18};

// Segment labelling:
//        A
//       ----
//     F |  | B
//       ---- G
//     E |  | C
//       ----   .H
//        D

#ifdef EZ_MODE
const byte ICACHE_FLASH_ATTR sevenSegmentDisplay[48][8] = {
  //H G F E D C B A
  { 0,0,1,1,0,0,0,0 },  // = 0
  { 0,0,1,1,0,0,0,1 },  // = 1
  { 0,1,1,1,0,0,0,0 },  // = 2
  { 0,1,1,1,0,0,0,1 },  // = 3
  { 0,0,1,1,1,0,0,0 },  // = 4
  { 0,0,1,1,1,0,0,1 },  // = 5
  { 0,1,1,1,1,0,0,0 },  // = 6
  { 0,0,1,1,0,1,1,1 },  // = 7
  { 0,0,0,0,0,1,1,1 },  // = 8
  { 0,1,0,0,0,1,1,0 },  // = 9
  { 0,1,0,0,0,1,1,1 },  // = A
  { 0,0,0,0,1,1,1,0 },  // = B
  { 0,0,0,0,1,1,1,1 },  // = C
  { 0,1,0,0,1,1,1,0 },  // = D
  { 0,1,1,1,1,1,1,1 },  // = E
  { 0,0,1,1,1,1,1,0 },  // = F
  { 0,0,0,0,0,0,0,0 },  // = Clear
  { 1,0,0,0,0,0,0,0 },  // = Clear. (With dot)
  { 0,1,0,0,0,0,0,0 },  // = -
  { 1,1,0,0,0,0,0,0 },  // = - . (With dot)
  { 1,0,1,1,0,0,0,0 },  // = 0.
  { 1,0,1,1,0,0,0,1 },  // = 1.
  { 1,1,1,1,0,0,0,0 },  // = 2.
  { 1,1,1,1,0,0,0,1 },  // = 3.
  { 1,0,1,1,1,0,0,0 },  // = 4.
  { 1,0,1,1,1,0,0,1 },  // = 5.
  { 1,1,1,1,1,0,0,0 },  // = 6.
  { 1,0,1,1,0,1,1,1 },  // = 7.
  { 1,0,0,0,0,1,1,1 },  // = 8.
  { 1,1,0,0,0,1,1,0 },  // = 9.
  { 1,1,0,0,0,1,1,1 },  // = A.
  { 1,0,0,0,1,1,1,0 },  // = B.
  { 1,0,0,0,1,1,1,1 },  // = C.
  { 1,1,0,0,1,1,1,0 },  // = D.
  { 1,1,1,1,1,1,1,1 },  // = E.
  { 1,0,1,1,1,1,1,0 },  // = F.
  { 0,0,1,1,0,0,0,1 },  // = R
  { 1,0,1,1,0,0,0,1 },  // = R.
  { 0,1,0,1,0,0,0,0 },  // = r
  { 1,1,0,1,0,0,0,0 },  // = r.
  { 0,1,0,1,1,1,0,0 },  // = o
  { 1,1,0,1,1,1,0,0 },  // = o.
  { 0,1,0,1,0,1,1,0 },  // = H
  { 1,1,0,1,0,1,1,0 },  // = H.
  { 0,0,1,1,0,1,1,0 },  // = 11 (X)
  { 1,0,1,1,0,1,1,0 },  // = 11 (X).
  { 0,1,1,1,1,0,0,1 },  // = E (Letter)
  { 1,1,1,1,1,0,0,1 }  // = E. (Letter)
};
#else
const byte ICACHE_FLASH_ATTR sevenSegmentDisplay[48][8] = {
  //H G F E D C B A
  { 0,0,1,1,1,1,1,1 },  // = 0
  { 0,0,0,0,0,1,1,0 },  // = 1
  { 0,1,0,1,1,0,1,1 },  // = 2
  { 0,1,0,0,1,1,1,1 },  // = 3
  { 0,1,1,0,0,1,1,0 },  // = 4
  { 0,1,1,0,1,1,0,1 },  // = 5
  { 0,1,1,1,1,1,0,1 },  // = 6
  { 0,0,0,0,0,1,1,1 },  // = 7
  { 0,1,1,1,1,1,1,1 },  // = 8
  { 0,1,1,0,0,1,1,1 },  // = 9
  { 0,1,1,1,0,1,1,1 },  // = A
  { 0,1,1,1,1,1,0,0 },  // = B
  { 0,0,1,1,1,0,0,1 },  // = C
  { 0,1,0,1,1,1,1,0 },  // = D
  { 0,1,1,1,1,0,0,1 },  // = E
  { 0,1,1,1,0,0,0,1 },  // = F
  { 0,0,0,0,0,0,0,0 },  // = Clear
  { 1,0,0,0,0,0,0,0 },  // = Clear. (With dot)
  { 0,1,0,0,0,0,0,0 },  // = -
  { 1,1,0,0,0,0,0,0 },  // = - . (With dot)
  { 0,0,1,1,1,1,1,1 },  // = 0.
  { 1,0,0,0,0,1,1,0 },  // = 1.
  { 1,1,0,1,1,0,1,1 },  // = 2.
  { 1,1,0,0,1,1,1,1 },  // = 3.
  { 1,1,1,0,0,1,1,0 },  // = 4.
  { 1,1,1,0,1,1,0,1 },  // = 5.
  { 1,1,1,1,1,1,0,1 },  // = 6.
  { 1,0,0,0,0,1,1,1 },  // = 7.
  { 1,1,1,1,1,1,1,1 },  // = 8.
  { 1,1,1,0,0,1,1,1 },  // = 9.
  { 1,1,1,1,0,1,1,1 },  // = A.
  { 1,1,1,1,1,1,0,0 },  // = B.
  { 1,0,1,1,1,0,0,1 },  // = C.
  { 1,1,0,1,1,1,1,0 },  // = D.
  { 1,1,1,1,1,0,0,1 },  // = E.
  { 1,1,1,1,0,0,0,1 },  // = F.
  { 0,0,1,1,0,0,0,1 },  // = R
  { 1,0,1,1,0,0,0,1 },  // = R.
  { 0,1,0,1,0,0,0,0 },  // = r
  { 1,1,0,1,0,0,0,0 },  // = r.
  { 0,1,0,1,1,1,0,0 },  // = o
  { 1,1,0,1,1,1,0,0 },  // = o.
  { 0,1,0,1,0,1,1,0 },  // = H
  { 1,1,0,1,0,1,1,0 },  // = H.
  { 0,0,1,1,0,1,1,0 },  // = 11 (X)
  { 1,0,1,1,0,1,1,0 },  // = 11 (X).
  { 0,1,1,1,1,0,0,1 },  // = E (Letter)
  { 1,1,1,1,1,0,0,1 }  // = E. (Letter)
};
#endif

const byte keyMap[16] = {
  1,
  2,
  3,
  10,
  4,
  5,
  6,
  11,
  7,
  8,
  9,
  12,
  14,
  0,
  15,
  13
};

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
byte webserverEnabled = 1;
int webserverListenPort = SERVER_LISTEN_PORT;

byte primaryEepromAddress = EEPROM_PRIMARY_DEFAULT_ADDR;
byte secondaryEepromAddress = EEPROM_SECONDARY_DEFAULT_ADDR;
byte primaryEepromReader = primaryEepromAddress;
byte secondaryEepromReader = secondaryEepromAddress;

byte lastKeyPress = NO_KEY;

TM1638 ioMod(DIO, CLK, STB);
ESP8266WebServer webServer(SERVER_LISTEN_PORT);

void Reset() {
  #ifdef IS_ATMEG
    asm volatile ("  jmp 0");
  #else
    ESP.reset();
  #endif
}

boolean ICACHE_FLASH_ATTR stringToNumber(int* res, char inputChar[]) {
  String strInputChar(inputChar);
  char *endPtr = NULL;
  if (strInputChar.indexOf(F("0x")) > -1) {
    *res = (int)strtol(inputChar, &endPtr, 16) % 0xFFFF;
  } else if (strInputChar.indexOf(F("0b")) > -1) {
    *res = (int)strtol(inputChar, &endPtr, 2) % 0xFFFF;
  } else {
    *res = (int)strtol(inputChar, &endPtr, 10) % 0xFFFF;
  }
  
  if (inputChar && !*endPtr) {
    return true;
  }
  return false;
}

unsigned char ICACHE_FLASH_ATTR bitsToChar(char *bitArr, unsigned int arrSize) {
  unsigned char res = 0;

  for (unsigned int i = 0; i < arrSize; i++) {
    res <<= 1;
    res += bitArr[i];
  }

  return res;
}

byte ICACHE_FLASH_ATTR hexToInt(char inputHex) {
  byte res = 0;

  if(inputHex <= 57) {
    res = inputHex - '0';
  } else {
    res = inputHex - 'A' + 10;
  }

  return res;
}

byte ICACHE_FLASH_ATTR getDigit(unsigned int inputNumber, byte loc, byte radix) {
  int res = 0;

  char numBuff[7] = {0};
  if (radix == 0x10) {
    sprintf(numBuff, "%4X", inputNumber);
  } else {
    sprintf(numBuff, "%4d", inputNumber);
  }

  if (radix == 0x10) {
    res = hexToInt(numBuff[(7 - (loc + 4))]);
  } else {
    res = (numBuff[(7 - (loc + 4))] - '0') % radix;
  }
  
  res &= 0xff;
  
  return (byte)res;
}

unsigned int ICACHE_FLASH_ATTR setDigit(unsigned int inputNumber, byte newDigit, byte pos, byte radix) {
  unsigned int posValue = 1;
  while (pos > 0) {
    posValue *= radix;
    --pos;
  }

  return inputNumber + ((int)newDigit - (int)((inputNumber / posValue) % radix)) * posValue;
}

void ICACHE_FLASH_ATTR exEepromWriteByte(int deviceAddress, unsigned int memAddress, byte data) {
  int rdata = data;
  delay(25);
  Wire.beginTransmission(deviceAddress);
  Wire.write((int)(memAddress >> 8)); // MSB
  Wire.write((int)(memAddress & 0xFF)); // LSB
  Wire.write(rdata & 0xFF);
  Wire.endTransmission();
  delay(25);
}

byte ICACHE_FLASH_ATTR exEepromReadByte(byte* res, int deviceAddress, unsigned int memAddress, byte defaultValue) {
  *res = defaultValue;
  delay(25);
  Wire.beginTransmission(deviceAddress);
  Wire.write((int)(memAddress >> 8)); // MSB
  Wire.write((int)(memAddress & 0xFF)); // LSB
  Wire.endTransmission();
  Wire.requestFrom(deviceAddress, 1);
  if (Wire.available()) {
    *res = Wire.read();
    delay(25);
    return 0;
  }
  delay(25);
  return 1;
}

byte ICACHE_FLASH_ATTR readStringFromEeprom(String* res, int deviceAddress, unsigned int startAddress, byte stringMaxLength) {
  byte stringIndex = 0;
  *res = "";
  char buf;
  while(stringIndex < stringMaxLength) {
    byte readResult = exEepromReadByte((byte *)&buf, deviceAddress, startAddress + stringIndex, EEPROM_READ_FAILURE);
    String temp(buf);
    if (buf == 0) {
      return 0;
    }
    if (buf < 32 || buf > 126 || readResult > 0) {
      return 1;
    }
    res->concat(buf);
    stringIndex++;
  }
  return 0;
}

byte ICACHE_FLASH_ATTR exEepromReadByte(byte* res, int deviceAddress, unsigned int memAddress) {
  return exEepromReadByte(res, deviceAddress, memAddress, (byte)EEPROM_READ_FAILURE);
}

void ICACHE_FLASH_ATTR clearScreen(bool ledsToo) {
  const byte clearDisplay[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    ioMod.setDisplay(clearDisplay);
    if (ledsToo) {
      ioMod.setLEDs(TM1638_COLOR_NONE);
    }
}

bool ICACHE_FLASH_ATTR readEepromSettings() {
  int eepromVersion = 0;
  
  exEepromReadByte((byte*)&eepromVersion, EEPROM_INTERNAL_ADDR, EE_READ_GOOD_LOC, EEPROM_READ_FAILURE);
  if (eepromVersion == EE_PROM_VERSION) {
    exEepromReadByte(&wifiRetryTimes, EEPROM_INTERNAL_ADDR, EE_MAX_WIFI_TRIES_LOC, wifiRetryTimes);
    exEepromReadByte(&enableSerialConn, EEPROM_INTERNAL_ADDR, EE_ENABLE_SERIAL_LOC, enableSerialConn);
    exEepromReadByte(&disableTransceiver, EEPROM_INTERNAL_ADDR, EE_DISABLE_TRANSCE_LOC, disableTransceiver);

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

unsigned char * breakIntToArray(int inputNumber, bool outputHex) {
  unsigned int modder = 0;
  
  if (outputHex) {
    modder = 16;
  } else {
    modder = 10;
  }

  unsigned char *numberArray = (unsigned char*)calloc(HALF_SEGMENT_COUNT, sizeof(char));

  unsigned int i = 0;
  unsigned int nCount = inputNumber;

  while (nCount != 0) {
    numberArray[i] = nCount % modder;
    nCount /= modder;
    i++;
  }

  i = HALF_SEGMENT_COUNT - 1;
  unsigned char j = 0;
  
  while(i > j) {
    int temp = numberArray[i];
    numberArray[i] = numberArray[j];
    numberArray[j] = temp;
    i--;
    j++;
  }

  return numberArray;
}

unsigned char * breakIntToArray(int inputNumber) {
  return breakIntToArray(inputNumber, true);
}

byte ICACHE_FLASH_ATTR decodeKeypad() {
  static bool noPress = 0;
  
  for(byte i = 0; i < pinCols; i++) {
    if (digitalRead(inputPins[i]) == HIGH);
    else
    break;
    if(i == (pinCols - 1)) {
      noPress = 1;
      scanR = 0;
      scanC = 0;
    }
  }
  
  if(noPress == 1) {
    for(byte i = 0; i < pinRows; i++) {
      digitalWrite(outputPins[i], LOW);
    }
    
    for(byte i = 0; i < pinCols; i++) {
      if(digitalRead(inputPins[i]) == HIGH) {
        continue;
      } else {
        for (scanR = 0; scanR < pinRows; scanR++) {
          digitalWrite(outputPins[scanR], HIGH);   
          if(digitalRead(inputPins[i]) == HIGH) {
            noPress = 0;               
            for(scanC = 0; scanC < pinRows; scanC++) {
              digitalWrite(outputPins[scanC], LOW);
            }
            return scanR * pinRows + i;  
          }
        }
      }
    }
  }

 return NO_KEY;
}

void ICACHE_FLASH_ATTR setNewAddress(unsigned int newAddress) {
  currentAddress = newAddress & 0xffff;
}

void ICACHE_FLASH_ATTR setupKeypadMatrixPins() {
  return;
  for(byte i = 0; i < pinRows; i++) {
    pinMode(outputPins[i],OUTPUT);
  }
  for(byte j = 0; j < pinCols; j++) {
    pinMode(inputPins[j],INPUT_PULLUP);
  }
}

void ICACHE_FLASH_ATTR updateScreen(unsigned char * value, boolean rightSide, bool rawMode) {
  int indexOffset = 0;
  indexOffset = rightSide ? HALF_SEGMENT_COUNT : 0;

  if (rawMode) {
    for (unsigned int i = 0; i < HALF_SEGMENT_COUNT; i++) {
      currentScreenValue[i + indexOffset] = value[i];
    }
  } else {
    for (unsigned int i = 0; i < HALF_SEGMENT_COUNT; i++) {
      currentScreenValue[i + indexOffset] = bitsToChar((char *)sevenSegmentDisplay[value[i]], 8);
    }
  }
}

void ICACHE_FLASH_ATTR updateScreen(unsigned char * value, boolean rightSide) {
  updateScreen(value, rightSide, false);
}

void ICACHE_FLASH_ATTR readAndDisplayAddress() {
  unsigned char *addrBuffer = breakIntToArray(currentAddress, modeHex);
  updateScreen(addrBuffer, false);
  
  if (addrBuffer) {
    free(addrBuffer);
  }
}

void ICACHE_FLASH_ATTR readAndDisplayMemory() {
  const byte readRes = exEepromReadByte(&memRead, primaryEepromReader, currentAddress, EEPROM_READ_FAILURE);
  unsigned char *memBuffer = breakIntToArray((byte)memRead, modeHex);
  if (readRes != 0) {
    updateScreen((unsigned char*)displayError, true);
    updateScreen((unsigned char*)NoRes, false);
  } else {
    updateScreen((unsigned char*)memBuffer, true);
  }
  
  if (memBuffer) {
    free(memBuffer);
  }
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
  
  unsigned char *addrBuffer = breakIntToArray(currentAddress, modeHex);
  updateScreen(addrBuffer, false);
  if (addrBuffer) {
    free(addrBuffer);
  }
}

void ICACHE_FLASH_ATTR numericAddressInputMode() {
  unsigned char *addrBuffer = breakIntToArray(currentAddress, modeHex);
  
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
    
    unsigned char *tempMemoryEditBuffer = breakIntToArray(tempMemoryEdit, modeHex);
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
    unsigned char *tempMemoryEditBuffer = breakIntToArray(tempMemoryEdit, modeHex);
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
    
    unsigned char *tempMemoryEditBuffer = breakIntToArray(tempMemoryEdit, modeHex);
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
    
    unsigned char *tempMemoryEditBuffer = breakIntToArray(tempMemoryEdit, modeHex);
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
    unsigned char *tempMemoryEditBuffer = breakIntToArray(tempMemoryEdit, modeHex);
    updateScreen(tempMemoryEditBuffer, true);
    if (tempMemoryEditBuffer) {
      free(tempMemoryEditBuffer);
    }
    setCurrentMode(BASIC_MEM_CTRL);
  }

  if (ioMod.getButtons() == 0b10000000) {
    exEepromWriteByte(primaryEepromReader, currentAddress, tempMemoryEdit);
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
    unsigned char *tempMemoryEditBuffer = breakIntToArray(tempMemoryEdit, modeHex);
    updateScreen(tempMemoryEditBuffer, true);
    if (tempMemoryEditBuffer) {
      free(tempMemoryEditBuffer);
    }
    readAndDisplayMemory();
  }

  if (ioMod.getButtons() == 0b00000001) {
    tempMemoryEdit++;
    unsigned char *tempMemoryEditBuffer = breakIntToArray(tempMemoryEdit, modeHex);
    updateScreen(tempMemoryEditBuffer, true);
    if (tempMemoryEditBuffer) {
      free(tempMemoryEditBuffer);
    }
  }

  if (ioMod.getButtons() == 0b00000010) {
    tempMemoryEdit--;
    unsigned char *tempMemoryEditBuffer = breakIntToArray(tempMemoryEdit, modeHex);
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
    exEepromWriteByte(primaryEepromReader, currentAddress, tempMemoryEdit);
    setCurrentMode(BASIC_MEM_CTRL);
  }
  
  if (ioMod.getButtons() == 0b01000000 || ioMod.getButtons() == 0b00100000) {
    tempMemoryEdit = memRead;
    unsigned char *tempMemoryEditBuffer = breakIntToArray(tempMemoryEdit, modeHex);
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

void parseSerialCommands(byte command, unsigned int *params, byte paramsLength) {
  char outputBuf[32];
  switch(command) {
    case OP_NOP: {
      Serial.println(F("nop"));
    }
    break;
    case OP_JMP: {
      if (serialEchoCommand) {
        Serial.print(F("jmp"));
        sprintf(outputBuf, " 0x%04x", params[0]);
        Serial.println(outputBuf);
      }
      if (paramsLength < 1) {
        Serial.println(F("err 2; jmp ("STR_OP_JMP") takes 1 param"));
        break;
      }
      setNewAddress(params[0]);
      sprintf(outputBuf, "0x%04x", params[0]);
      Serial.println(outputBuf);
    }
    break;
    case OP_AIC: {
      if (serialEchoCommand) {
        Serial.println(F("aic"));
      }
      setNewAddress(currentAddress + 1);
      sprintf(outputBuf, "0x%04x", currentAddress);
      Serial.println(outputBuf);
    }
    break;
    case OP_ADC: {
      if (serialEchoCommand) {
        Serial.println(F("adc"));
      }
      setNewAddress(currentAddress - 1);
      sprintf(outputBuf, "0x%04x", currentAddress);
      Serial.println(outputBuf);
    }
    break;
    case OP_GETC: {
      if (paramsLength == 0) {
        if (serialEchoCommand) {
          Serial.println(F("get"));
        }
        sprintf(outputBuf, "0x%04x", currentAddress);
        Serial.print(outputBuf);
        sprintf(outputBuf, " 0x%02x", memRead);
        Serial.println(outputBuf);
      }
    }
    break;
    case OP_GET: {
      if (paramsLength == 1) {
        if (serialEchoCommand) {
          Serial.print(F("get "));
          sprintf(outputBuf, "0x%04x", params[0]);
          Serial.println(outputBuf);
        }
        const byte readRes = exEepromReadByte(&memRead, primaryEepromReader, params[0], EEPROM_READ_FAILURE);
        sprintf(outputBuf, "0x%04x", params[0]);
        Serial.print(outputBuf);
        sprintf(outputBuf, " 0x%02x", memRead);
        Serial.println(outputBuf);
      } else {
        Serial.println(F("err 3; get ("STR_OP_GET") takes 1 param"));
        break;
      }
    }
    break;
    case OP_MOV: {
      if (serialEchoCommand) {
        Serial.print(F("mov "));
        sprintf(outputBuf, "0x%04x", params[0]);
        Serial.print(outputBuf);
        sprintf(outputBuf, " 0x%02x", params[1] & 0xFF);
        Serial.println(outputBuf);
      }
      if (paramsLength == 2) {
        setNewAddress(params[0]);
        exEepromWriteByte(primaryEepromReader, currentAddress, params[1] & 0xFF);
        const byte readRes = exEepromReadByte(&memRead, primaryEepromReader, currentAddress, EEPROM_READ_FAILURE);
        sprintf(outputBuf, "0x%04x", currentAddress);
        Serial.print(outputBuf);
        sprintf(outputBuf, " 0x%02x", memRead);
        Serial.println(outputBuf);
      } else {
        Serial.println(F("err 4; mov ("STR_OP_MOV") takes 2 params"));
        break;
      }
    }
    break;
    case OP_L2M: {
      if (serialEchoCommand) {
        Serial.print(F("l2m "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        Serial.println(outputBuf);
      }
      if (paramsLength == 1) {
        exEepromWriteByte(primaryEepromReader, currentAddress, params[0] & 0xFF);
        const byte readRes = exEepromReadByte(&memRead, primaryEepromReader, currentAddress, EEPROM_READ_FAILURE);
        sprintf(outputBuf, "0x%04x", currentAddress);
        Serial.print(outputBuf);
        sprintf(outputBuf, " 0x%02x", memRead);
        Serial.println(outputBuf);
      } else {
        Serial.println(F("err 8; "STR_OP_L2M" takes 1 params"));
        break;
      }
    }
    break;
    case OP_PI2CC: {
      if (paramsLength == 0) {
        if (serialEchoCommand) {
          Serial.println(F("pi2c"));
        }
        sprintf(outputBuf, "0x%02x", secondaryEepromAddress);
        Serial.println(outputBuf);
      }
    }
    break;
    case OP_SI2CC: {
      if (paramsLength == 0) {
        if (serialEchoCommand) {
          Serial.println(F("si2c"));
        }
        sprintf(outputBuf, "0x%02x", secondaryEepromAddress);
        Serial.println(outputBuf);
      }
    }
    break;
    case OP_PI2C: {
      if (serialEchoCommand) {
        Serial.print(F("pi2c "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        Serial.println(outputBuf);
      }
      if (paramsLength == 1) {
        primaryEepromAddress = params[0];
        sprintf(outputBuf, "0x%02x", params[0]);
        Serial.println(outputBuf);
        delay(10);
      } else {
        Serial.println(F("err 16; "STR_OP_PI2C" takes 1 params"));
        break;
      }
    }
    break;
    case OP_SI2C: {
      if (serialEchoCommand) {
        Serial.print(F("si2c "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        Serial.println(outputBuf);
      }
      if (paramsLength == 1) {
        secondaryEepromAddress = params[0];
        sprintf(outputBuf, "0x%02x", params[0]);
        Serial.println(outputBuf);
        delay(10);
      } else {
        Serial.println(F("err 16; "STR_OP_SI2C" takes 1 params"));
        break;
      }
    }
    break;
    case OP_PERC: {
      if (paramsLength == 0) {
        if (serialEchoCommand) {
          Serial.println(F("per"));
        }
        sprintf(outputBuf, "0x%02x", primaryEepromReader);
        Serial.println(outputBuf);
      }
    }
    break;
    case OP_PER: {
      if (serialEchoCommand) {
        Serial.print(F("per "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        Serial.println(outputBuf);
      }
      if (paramsLength == 1) {
        if (params[0] == 0x00) {
          primaryEepromReader = EEPROM_INTERNAL_ADDR;
        } else if (params[0] == 0x01) {
          primaryEepromReader = primaryEepromAddress;
        } else if (params[0] == 0x02) {
          primaryEepromReader = secondaryEepromAddress;
        } else {
          primaryEepromReader = primaryEepromAddress;
        }
        sprintf(outputBuf, "0x%02x", params[0]);
        Serial.print(outputBuf);
        Serial.print(F("; "));
        sprintf(outputBuf, "0x%02x", primaryEepromReader);
        Serial.println(outputBuf);
        delay(10);
      } else {
        Serial.println(F("err 16; "STR_OP_PER" takes 1 params"));
        break;
      }
    }
    break;
    case OP_SERC: {
      if (paramsLength == 0) {
        if (serialEchoCommand) {
          Serial.println(F("ser"));
        }
        sprintf(outputBuf, "0x%02x", secondaryEepromReader);
        Serial.println(outputBuf);
      }
    }
    break;
    case OP_SER: {
      if (serialEchoCommand) {
        Serial.print(F("ser "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        Serial.println(outputBuf);
      }
      if (paramsLength == 1) {
        if (params[0] == 0x00) {
          secondaryEepromReader = EEPROM_INTERNAL_ADDR;
        } else if (params[0] == 0x01) {
          secondaryEepromReader = primaryEepromAddress;
        } else if (params[0] == 0x02) {
          secondaryEepromReader = secondaryEepromAddress;
        } else {
          secondaryEepromReader = secondaryEepromAddress;
        }
        sprintf(outputBuf, "0x%02x", params[0]);
        Serial.print(outputBuf);
        Serial.print(F("; "));
        sprintf(outputBuf, "0x%02x", secondaryEepromReader);
        Serial.println(outputBuf);
        delay(10);
      } else {
        Serial.println(F("err 16; "STR_OP_SER" takes 1 params"));
        break;
      }
    }
    break;
    case OP_PCPY: {
      if (serialEchoCommand) {
        Serial.print(F("pcpy "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        Serial.println(outputBuf);
      }
      if (paramsLength == 2) {
        const unsigned int startAddress = currentAddress;
        const int maxCopySize = 4096;
        unsigned int memAddrOffset = 0;
        byte tmpMem = 0;
        
        for (memAddrOffset = 0; memAddrOffset < maxCopySize && memAddrOffset < params[1]; memAddrOffset++) {
          setNewAddress(startAddress + memAddrOffset);
          readAndDisplayMemory();
          readAndDisplayAddress();
          const byte readRes = exEepromReadByte(&tmpMem, primaryEepromReader, currentAddress, EEPROM_READ_FAILURE);
          exEepromWriteByte(secondaryEepromReader, currentAddress, params[0] & 0xFF);
          ioMod.setDisplay(currentScreenValue);
          sprintf(outputBuf, "0x%02x ", tmpMem);
          Serial.print(outputBuf);
        }
        
        Serial.print('\n');
        sprintf(outputBuf, "0x%02x", memAddrOffset);
        Serial.println(outputBuf);
        delay(10);
      } else {
        Serial.println(F("err 16; "STR_OP_INT" takes 2 params"));
        break;
      }
    }
    break;
    case OP_INT: {
      if (serialEchoCommand) {
        Serial.print(F("int "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        Serial.println(outputBuf);
      }
      if (paramsLength == 1) {
        assemblyInterpretMode = params[0] == 0 ? false : true;
        sprintf(outputBuf, "0x%02x", params[0]);
        Serial.print(outputBuf);
        Serial.print("; ");
        Serial.println(assemblyInterpretMode ? F("true") : F("false"));
        delay(10);
      } else {
        Serial.println(F("err 16; "STR_OP_INT" takes 1 params"));
        break;
      }
    }
    break;
    case OP_ECH: {
      if (serialEchoCommand) {
        Serial.print(F("ech "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        Serial.println(outputBuf);
      }
      if (paramsLength == 1) {
        serialEchoCommand = params[0] == 0 ? false : true;
        sprintf(outputBuf, "0x%02x", params[0]);
        Serial.print(outputBuf);
        Serial.print(F("; "));
        Serial.println(serialEchoCommand ? F("true") : F("false"));
        delay(10);
      } else {
        Serial.println(F("err 32; "STR_OP_ECH" takes 1 params"));
        break;
      }
    }
    break;
    case OP_ADMP: {
      if (serialEchoCommand) {
        Serial.print(F("admp "));
        sprintf(outputBuf, "0x%04x", params[0]);
        Serial.print(outputBuf);
        sprintf(outputBuf, " 0x%02x", params[1] & 0xFF);
        Serial.println(outputBuf);
      }
      if (paramsLength == 2) {
        setNewAddress(params[0]);
        const int maxCopySize = 4096;
        unsigned int memAddrOffset = 0;
        byte tmpMemVal = 0;
        
        for (memAddrOffset = 0; memAddrOffset < maxCopySize && memAddrOffset < params[1]; memAddrOffset++) {
          setNewAddress(params[0] + memAddrOffset);
          readAndDisplayMemory();
          readAndDisplayAddress();
          const byte readRes = exEepromReadByte(&tmpMemVal, primaryEepromReader, currentAddress, EEPROM_READ_FAILURE);
          ioMod.setDisplay(currentScreenValue);
          sprintf(outputBuf, "0x%02x ", tmpMemVal);
          Serial.print(outputBuf);
        }
        
        Serial.print('\n');
        sprintf(outputBuf, "0x%02x", memAddrOffset);
        Serial.println(outputBuf);
        delay(10);
      } else {
        Serial.println(F("err 32; "STR_OP_ADMP" takes 2 params"));
        break;
      }
    }
    break;
    case OP_CDMP: {
      if (serialEchoCommand) {
        Serial.print(F("cdmp "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        Serial.println(outputBuf);
      }
      if (paramsLength == 1) {
        const unsigned int startAddress = currentAddress;
        const int maxCopySize = 4096;
        unsigned int memAddrOffset = 0;
        byte tmpMemVal = 0;
        
        for (memAddrOffset = 0; memAddrOffset < maxCopySize && memAddrOffset < params[0]; memAddrOffset++) {
          setNewAddress(startAddress + memAddrOffset);
          readAndDisplayMemory();
          readAndDisplayAddress();
          const byte readRes = exEepromReadByte(&tmpMemVal, primaryEepromReader, currentAddress, EEPROM_READ_FAILURE);
          ioMod.setDisplay(currentScreenValue);
          sprintf(outputBuf, "0x%02x ", tmpMemVal);
          Serial.print(outputBuf);
        }
        
        Serial.print('\n');
        sprintf(outputBuf, "0x%02x", memAddrOffset);
        Serial.println(outputBuf);
        delay(10);
      } else {
        Serial.println(F("err 32; "STR_OP_CDMP" takes 1 params"));
        break;
      }
    }
    break;
    case OP_RETD: {
      if (serialEchoCommand) {
        Serial.println(F("ret"));
      }
      Serial.print(F("ret "));
      sprintf(outputBuf, "0x%02x", DEFAULT_RUN_MODE);
      Serial.println(outputBuf);
      delay(10);
      currentMode = DEFAULT_RUN_MODE;
      ioMod.setLED(TM1638_COLOR_RED, 0);
      ioMod.setLED(TM1638_COLOR_RED, 1);
      ioMod.setLED(TM1638_COLOR_RED, 2);
      ioMod.setLED(TM1638_COLOR_RED, 3);
    }
    break;
    case OP_RET: {
      if (paramsLength == 1) {
        if (serialEchoCommand) {
          Serial.print(F("ret "));
          sprintf(outputBuf, "0x%02x", params[0]);
          Serial.println(outputBuf);
        }
        Serial.print(F("ret "));
        sprintf(outputBuf, "0x%02x", params[0]);
        Serial.println(outputBuf);
        delay(10);
        currentMode = params[0];
      } else {
        Serial.println(F("err 64; ret ("STR_OP_RET") takes 1 params"));
        break;
      }
    }
    break;
    case OP_I2C: {
      if (serialEchoCommand) {
        Serial.println(F("i2c"));
      }
      const byte totalDeviceCount = 127;
      int goodDevices[totalDeviceCount];
      int badDevices[totalDeviceCount];
      byte goodFoundDevicesLength = 0;
      byte badFoundDevicesLength = 0;

      scanI2CDevices(goodDevices, (int)totalDeviceCount, &goodFoundDevicesLength, badDevices, (int)totalDeviceCount, &badFoundDevicesLength);

      if (goodFoundDevicesLength > 0) {
        Serial.print(F("Good: "));
        for (int i = 0; i < goodFoundDevicesLength ; i++) {
          sprintf(outputBuf, "0x%02x ", goodDevices[i]);
          Serial.print(outputBuf);
        }
        Serial.print('\n');
      }

      if (badFoundDevicesLength > 0) {
        Serial.print(F("Bad: "));
        for (int i = 0; i < badFoundDevicesLength ; i++) {
          sprintf(outputBuf, "0x%02 ", badDevices[i]);
          Serial.print(outputBuf);
        }
        Serial.print('\n');
      }
    }
    break;
    case OP_RST: {
      if (serialEchoCommand) {
        Serial.println(F("rst"));
      }
      Serial.println(F("RESET"));
      delay(10);
      Reset();
    }
    break;
  }
  readAndDisplayAddress();
  readAndDisplayMemory();
  ioMod.setDisplay(currentScreenValue);
}

void ICACHE_FLASH_ATTR commandDecode(byte ret[], char currentToken[], unsigned int params[], byte paramsLength, char remaining[], int remainingLength) {
  ret[0] = OP_NOP;
  ret[1] = 0;

  if (strcmp(currentToken, STR_OP_NOP) == 0) {
    ret[0] = OP_NOP;
  } else if (strcmp(currentToken, STR_OP_JMP) == 0) {
    ret[0] = OP_JMP;
    ret[1] = 1;
  } else if (strcmp(currentToken, STR_OP_AIC) == 0) {
    ret[0] = OP_AIC;
  } else if (strcmp(currentToken, STR_OP_ADC) == 0) {
    ret[0] = OP_ADC;
  } else if (strcmp(currentToken, STR_OP_GETC) == 0) {
    ret[0] = OP_GETC;
  } else if (strcmp(currentToken, STR_OP_GET) == 0) {
    ret[0] = OP_GET;
    ret[1] = 1;
  } else if (strcmp(currentToken, STR_OP_MOV) == 0) {
    ret[0] = OP_MOV;
    ret[1] = 2;
  } else if (strcmp(currentToken, STR_OP_L2M) == 0) {
    ret[0] = OP_L2M;
    ret[1] = 1;
  } else if (strcmp(currentToken, STR_OP_PI2CC) == 0) {
    ret[0] = OP_PI2CC;
    ret[1] = 1;
  } else if (strcmp(currentToken, STR_OP_SI2CC) == 0) {
    ret[0] = OP_SI2CC;
    ret[1] = 1;
  } else if (strcmp(currentToken, STR_OP_PI2C) == 0) {
    ret[0] = OP_PI2C;
  } else if (strcmp(currentToken, STR_OP_SI2C) == 0) {
    ret[0] = OP_SI2C;
  } else if (strcmp(currentToken, STR_OP_PERC) == 0) {
    ret[0] = OP_PERC;
    ret[1] = 1;
  } else if (strcmp(currentToken, STR_OP_PER) == 0) {
    ret[0] = OP_PER;
  } else if (strcmp(currentToken, STR_OP_SERC) == 0) {
    ret[0] = OP_SERC;
    ret[1] = 1;
  } else if (strcmp(currentToken, STR_OP_SER) == 0) {
    ret[0] = OP_SER;
  } else if (strcmp(currentToken, STR_OP_PCPY) == 0) {
    ret[0] = OP_PCPY;
    ret[1] = 2;
  } else if (strcmp(currentToken, STR_OP_INT) == 0) {
    ret[0] = OP_INT;
    ret[1] = 1;
  } else if (strcmp(currentToken, STR_OP_ECH) == 0) {
    ret[0] = OP_ECH;
    ret[1] = 1;
  } else if (strcmp(currentToken, STR_OP_RETD) == 0) {
    ret[0] = OP_RETD;
  } else if (strcmp(currentToken, STR_OP_ADMP) == 0) {
    ret[0] = OP_ADMP;
    ret[1] = 2;
  } else if (strcmp(currentToken, STR_OP_CDMP) == 0) {
    ret[0] = OP_CDMP;
    ret[1] = 1;
  } else if (strcmp(currentToken, STR_OP_I2C) == 0) {
    ret[0] = OP_I2C;
  } else if (strcmp(currentToken, STR_OP_RET) == 0) {
    ret[0] = OP_RET;
    ret[1] = 1;
  } else if (strcmp(currentToken, STR_OP_RST) == 0) {
    ret[0] = OP_RST;
  } else if (strcmp(currentToken, "nop") == 0) {
    ret[0] = OP_NOP;
  } else if (strcmp(currentToken, "jmp") == 0) {
    ret[0] = OP_JMP;
    ret[1] = 1;
  } else if (strcmp(currentToken, "aic") == 0 || strcmp(currentToken, "inc") == 0) {
    ret[0] = OP_AIC;
  } else if (strcmp(currentToken, "adc") == 0 || strcmp(currentToken, "dec") == 0) {
    ret[0] = OP_ADC;
  } else if (strcmp(currentToken, "get") == 0) {
    ret[0] = OP_GET;
    ret[1] = 1;
    if (remainingLength == 0) {
      ret[0] = OP_GETC;
      ret[1] = 0;
    }
  } else if (strcmp(currentToken, "mov") == 0) {
    ret[0] = OP_MOV;
    ret[1] = 2;
  } else if (strcmp(currentToken, "l2m") == 0) {
    ret[0] = OP_L2M;
    ret[1] = 1;
  } else if (strcmp(currentToken, "pi2c") == 0) {
    ret[0] = OP_PI2C;
    ret[1] = 1;
    if (remainingLength == 0) {
      ret[0] = OP_PI2CC;
      ret[1] = 0;
    }
  } else if (strcmp(currentToken, "si2c") == 0) {
    ret[0] = OP_SI2C;
    ret[1] = 1;
    if (remainingLength == 0) {
      ret[0] = OP_SI2CC;
      ret[1] = 0;
    }
  } else if (strcmp(currentToken, "per") == 0) {
    ret[0] = OP_PER;
    ret[1] = 1;
    if (remainingLength == 0) {
      ret[0] = OP_PERC;
      ret[1] = 0;
    }
  } else if (strcmp(currentToken, "ser") == 0) {
    ret[0] = OP_SER;
    ret[1] = 1;
    if (remainingLength == 0) {
      ret[0] = OP_SERC;
      ret[1] = 0;
    }
  } else if (strcmp(currentToken, "pcpy") == 0) {
    ret[0] = OP_PCPY;
    ret[1] = 2;
  } else if (strcmp(currentToken, "int") == 0) {
    ret[0] = OP_INT;
    ret[1] = 1;
  } else if (strcmp(currentToken, "ech") == 0) {
    ret[0] = OP_ECH;
    ret[1] = 1;
  } else if (strcmp(currentToken, "admp") == 0) {
    ret[0] = OP_ADMP;
    ret[1] = 2;
  } else if (strcmp(currentToken, "cdmp") == 0) {
    ret[0] = OP_CDMP;
    ret[1] = 1;
  } else if (strcmp(currentToken, "i2c") == 0) {
    ret[0] = OP_I2C;
  } else if (strcmp(currentToken, "ret") == 0) {
    ret[0] = OP_RET;
    ret[1] = 1;
    if (remainingLength == 0) {
      ret[0] = OP_RETD;
      ret[1] = 0;
    }
  } else if (strcmp(currentToken, "rst") == 0) {
    ret[0] = OP_RST;
  } else {
    char outputBuf[32];
    String strCurrentToken(currentToken);
    Serial.print(F("err 1; Unknown OP."));
    Serial.print(F(" Len: ["));
    sprintf(outputBuf, "0x%02x", strCurrentToken.length());
    Serial.print(outputBuf);
    Serial.print(F("]. Chars: ["));
    for (byte i = 0; i < strCurrentToken.length() - 1  && i < 0xFFFF; i++) {
      sprintf(outputBuf, "0x%02x", (byte)currentToken[i]);
      Serial.print(outputBuf);
      Serial.print(", ");
    }
    sprintf(outputBuf, "0x%02x", (byte)currentToken[strCurrentToken.length() - 1]);
    Serial.print(outputBuf);
    Serial.print("]: ");
    Serial.println(currentToken);
  }
}

void ICACHE_FLASH_ATTR commandParamParse(char currentToken[], unsigned int params[], byte* paramsLength) {
  String strCurrentToken(currentToken);
  if (*paramsLength < 2) { // Ignore extras
    unsigned int res;
    stringToNumber((int *)&res, currentToken);
    params[*paramsLength] = res;
    *paramsLength++;
  }
}

void ICACHE_FLASH_ATTR processCommandInputByteCode(char* byteCodeArr, int byteCodeLength) {
  char *remaining;
  char *currentToken;
  
  byte paramsLength = 0;
  byte commandDecodeRet[2] = {0};
  unsigned int params[2] = {0};

  if (commandDecodeRet[0] > 0) {
    // Command parameter parser
    commandParamParse(currentToken, params, &paramsLength);

    if (paramsLength >= commandDecodeRet[1]) {
      parseSerialCommands(commandDecodeRet[0], params, paramsLength);
      commandDecodeRet[0] = 0;
      commandDecodeRet[1] = 0;
      paramsLength = 0;
      params[0] = 0;
      params[1] = 0;
    }
  } else {
    // Command decoder
    commandDecode(commandDecodeRet, currentToken, params, paramsLength, remaining, strlen(remaining));

    if (commandDecodeRet[1] == 0) {
      parseSerialCommands(commandDecodeRet[0], params, paramsLength);
      commandDecodeRet[0] = 0;
      commandDecodeRet[1] = 0;
      paramsLength = 0;
      params[0] = 0;
      params[1] = 0;
    }
  }
}

void ICACHE_FLASH_ATTR processCommandInput(char* commSeg, bool assemblyMode) {
  char *remaining;
  char *currentToken;
  
  byte paramsLength = 0;
  byte commandDecodeRet[2] = {0};
  unsigned int params[2] = {0};

  if (assemblyMode) {
    while ((remaining = strtok_r(commSeg, ";", &commSeg)) != NULL) {
      commandDecodeRet[0] = 0;
      commandDecodeRet[1] = 0;
      paramsLength = 0;
      params[0] = 0;
      params[1] = 0;
      while ((currentToken = strtok_r(remaining, " ", &remaining)) != NULL) {
        String strRemaining(remaining);
        if (commandDecodeRet[0] > 0) {
          // Command parameter parser
          commandParamParse(currentToken, params, &paramsLength);
          if (strRemaining.length() == 0) {
            parseSerialCommands(commandDecodeRet[0], params, paramsLength);
          }
        } else {
          // Command decoder
          commandDecode(commandDecodeRet, currentToken, params, paramsLength, remaining, strlen(remaining));

          if (strRemaining.length() == 0) {
            parseSerialCommands(commandDecodeRet[0], params, paramsLength);
          }
        }
      }
    }
  } else {
    remaining = commSeg;
    while ((currentToken = strtok_r(remaining, " ", &remaining)) != NULL) {
      ////////
    }
  }
}

void ICACHE_FLASH_ATTR processCommandInput(char* commSeg) {
  processCommandInput(commSeg, assemblyInterpretMode);
}

void ICACHE_FLASH_ATTR serialCommandInput() {
  if (Serial.available()) {
    String serialResponse = Serial.readStringUntil('\n');
    serialResponse.trim();
    char *commSeg = (char *)serialResponse.c_str();
    processCommandInput(commSeg); 
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

void ICACHE_FLASH_ATTR updateScreenConnect(int currentTry) {
  for (byte i = 0, j = 7; i < 8; i++, j--) {
    ioMod.setLED(TM1638_COLOR_NONE, j);
  }

  ioMod.setLED(TM1638_COLOR_RED, (currentTry % 8));
}

void ICACHE_FLASH_ATTR httpHandleRoot() {
  DynamicJsonDocument JSONbuffer(1024);
  char JSONmessageBuffer[1024];
  
  JsonObject JSONencoder = JSONbuffer.to<JsonObject>();
  JSONencoder[F("payload")];
  JSONencoder[F("payload")][F("meta")];
  JSONencoder[F("payload")][F("data")];

  JSONencoder[F("payload")][F("meta")][F("uptime")] = millis();

  serializeJson(JSONencoder, JSONmessageBuffer);
  
  webServer.send(200, "application/json", JSONmessageBuffer);
}

void ICACHE_FLASH_ATTR httpHandleIntError(int locIndex, char* errorChars, char* errorMessage) {
    StaticJsonDocument<512> JSONbuffer;
    char JSONmessageBuffer[512];
    
    JsonObject JSONencoder = JSONbuffer.to<JsonObject>();
    JSONencoder[F("payload")];
    JSONencoder[F("payload")][F("meta")];
    JSONencoder[F("payload")][F("status")];
  
    JSONencoder[F("payload")][F("meta")][F("uptime")] = millis();
  
    JSONencoder[F("payload")][F("status")][F("code")] = F("400");
    JSONencoder[F("payload")][F("status")][F("codeMessage")] = F("Bad Request");
    JSONencoder[F("payload")][F("status")][F("errorDescription")] = errorMessage;
    JSONencoder[F("payload")][F("status")][F("location")] = locIndex;
    JSONencoder[F("payload")][F("status")][F("characters")] = errorChars;
    
    serializeJson(JSONencoder, JSONmessageBuffer);
    
    webServer.send(400, "application/json", JSONmessageBuffer);
}

void ICACHE_FLASH_ATTR httpHandleInt() {
  if (webServer.hasArg("plain") == false) {
    StaticJsonDocument<256> JSONbuffer;
    char JSONmessageBuffer[256];
    
    JsonObject JSONencoder = JSONbuffer.to<JsonObject>();
    JSONencoder[F("payload")];
    JSONencoder[F("payload")][F("meta")];
    JSONencoder[F("payload")][F("status")];
  
    JSONencoder[F("payload")][F("meta")][F("uptime")] = millis();
  
    JSONencoder[F("payload")][F("status")][F("code")] = F("400");
    JSONencoder[F("payload")][F("status")][F("codeMessage")] = F("Bad Request");
  
    serializeJson(JSONencoder, JSONmessageBuffer);
    
    webServer.send(400, "application/json", JSONmessageBuffer);
    return;
  }

  DynamicJsonDocument JSONbuffer(1024);
  deserializeJson(JSONbuffer, webServer.arg("plain"));

  JsonArray opList = JSONbuffer["ops"];
  byte codeList[OP_PROCESS_ARR_LIMIT];
  int byteIndex = 0;

  for(JsonVariant elem: opList) {
    if (byteIndex > OP_PROCESS_ARR_LIMIT) {
      char* errorMsg = strdup((const char*)F("Byte array length exceeds " OP_PROCESS_ARR_LIMIT));
      httpHandleIntError(byteIndex, (char *)elem.as<int>(), errorMsg);
      return;
    }
    if (elem.is<char*>()) {
      unsigned int res;
      char* inputChar = strdup(elem.as<char*>());
      boolean numRes = stringToNumber((int *)&res, inputChar);
      if (!numRes) {
        char* errorMsg = strdup((const char*)F("Invalid character in OP list"));
        httpHandleIntError(byteIndex, inputChar, errorMsg);
        return;
      }
      codeList[byteIndex] = res;
      byteIndex++;
    } else if (elem.is<byte>()) {
      codeList[byteIndex] = elem.as<byte>();
      byteIndex++;
    } else if (elem.is<int>()) {
      int res = elem.as<int>();
      if (res > 0xFFFF) {
        char* errorMsg = strdup((const char*)F("Input number exceeds 0xFFFF (65535)"));
        httpHandleIntError(byteIndex, (char *)elem.as<int>(), errorMsg);
        return;
      }
      codeList[byteIndex] = res & 0xFFFF;
      byteIndex++;
    } else {
      StaticJsonDocument<512> JSONbuffer;
      char JSONmessageBuffer[512];
      
      JsonObject JSONencoder = JSONbuffer.to<JsonObject>();
      JSONencoder[F("payload")];
      JSONencoder[F("payload")][F("meta")];
      JSONencoder[F("payload")][F("status")];
    
      JSONencoder[F("payload")][F("meta")][F("uptime")] = millis();
    
      JSONencoder[F("payload")][F("status")][F("code")] = F("400");
      JSONencoder[F("payload")][F("status")][F("codeMessage")] = F("Bad Request");
      JSONencoder[F("payload")][F("status")][F("errorDescription")] = F("OPs array does not only contain String<byte> or int types.");
      JSONencoder[F("payload")][F("status")][F("goodExample")] = F("{'ops': ['0x04', '0x05', 5, 7]}");
      JSONencoder[F("payload")][F("status")][F("badExample")] = F("{'ops': [0x04, 0x05, A, B, [1, 2], {'some': 'obj'} ]}");
    
      serializeJson(JSONencoder, JSONmessageBuffer);
      
      webServer.send(400, "application/json", JSONmessageBuffer);
      return;
    }

    
  }

//  char  

  webServer.send(200, "application/json", "hi");
}

void ICACHE_FLASH_ATTR httpHandleNotFound() {
  StaticJsonDocument<256> JSONbuffer;
  char JSONmessageBuffer[256];
  
  JsonObject JSONencoder = JSONbuffer.to<JsonObject>();
  JSONencoder[F("payload")];
  JSONencoder[F("payload")][F("meta")];
  JSONencoder[F("payload")][F("status")];

  JSONencoder[F("payload")][F("meta")][F("uptime")] = millis();

  JSONencoder[F("payload")][F("status")][F("code")] = F("404");
  JSONencoder[F("payload")][F("status")][F("codeMessage")] = F("Not Found");

  serializeJson(JSONencoder, JSONmessageBuffer);
  
  webServer.send(404, "application/json", JSONmessageBuffer);
}

void setup() {
  Wire.begin();

  readEepromSettings();

  if (enableSerialConn == 1) {
    Serial.begin(serialConnSpeed);
    Serial.println(F("Device booting"));
  }
  
  setupKeypadMatrixPins();
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
      webServer.on("/exec", httpHandleRoot);
      webServer.on("/int", httpHandleInt);
      webServer.onNotFound(httpHandleNotFound);
      webServer.begin();
    }

    if (mdnsEnabled == 1) {
      if (MDNS.begin(mdnsName)) {
        Serial.println("MDNS responder started");
      } else {
        Serial.println("Failed to start MDNS responder");
      }
    }
  }

  setCurrentMode(DEFAULT_RUN_MODE);
  
  readAndDisplayAddress();
  readAndDisplayMemory();
  ioMod.setDisplay(currentScreenValue);
  
  if (Serial.available()) {
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

  lastKeyPress = decodeKeypad();
  
//  readEepromSettings(); // Very slow

  if (enableSerialConn == 1) {
    if (!Serial) {
      Serial.begin(serialConnSpeed);
      if (Serial.available()) {
        Serial.println("Serial Listening...");
      }
    }
    
    if (Serial.available()) {
      serialCommandInput();
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

  if (serialDebugOutput && currentMode != SERIAL_COMMAND) {
    Serial.print("\n \nScreen Data: ");
    Serial.print(currentScreenValue[0], HEX);
    Serial.print(currentScreenValue[1], HEX);
    Serial.print(currentScreenValue[2], HEX);
    Serial.print(currentScreenValue[3], HEX);
    Serial.print(currentScreenValue[4], HEX);
    Serial.print(currentScreenValue[5], HEX);
    Serial.print(currentScreenValue[6], HEX);
    Serial.print(currentScreenValue[7], HEX);
    Serial.print(currentScreenValue[8], HEX);
    Serial.print(F("\nCurrent Address: "));
    Serial.print(currentAddress, HEX);
    Serial.print(F("\nCurrent Memory: "));
    Serial.print(memRead, HEX);
    Serial.print(F("\nCurrent Mode: "));
    Serial.print(currentMode, HEX);
    if (lastKeyPress != NO_KEY) {
      Serial.print(F("\nLast Keypin Pressed: "));
      Serial.println(lastKeyPress, HEX);
    }
    Serial.println(" ");
  }
}
