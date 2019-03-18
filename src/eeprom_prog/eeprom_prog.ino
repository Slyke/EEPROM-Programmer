#include <Wire.h>
#include <TM1638.h>

#define STB 2
#define CLK 3
#define DIO 4

#define NO_KEY 0x32

#define SEGMENT_COUNT 8
#define HALF_SEGMENT_COUNT SEGMENT_COUNT / 2

#define BASIC_ADDR_INPUT 0
#define NUMERIC_ADDR_INPUT 1
#define BASIC_MEM_CTRL 2
#define BASIC_MEM_INPUT 3
#define NUMERIC_MEM_INPUT 4

#define EEPROM_ADDR 0x50 // I2C Address
#define EEPROM_READ_FAILURE 0xFF // Returned value on fail. Don't set to 0, 1 or EE_PROM_VERSION

#define REFRESH_SPEED 150

const byte pinRows = 4;
const byte pinCols = 4;
const byte outputPins[pinRows] = {13, 12, 11, 10};
const byte inputPins[pinCols] = {9, 8, 7, 6};
const unsigned char displayError[4] = {14, 38, 40, 38};
const unsigned char displayDec[4] = {16, 13, 14, 12};
const unsigned char displayHex[4] = {16, 42, 14, 44};

// Segment labelling:
//        A
//       ----
//     F |  | B
//       ---- G
//     E |  | C
//       ----   .H
//        D

const byte sevenSegmentDisplay[46][8] = {
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
  { 1,0,1,1,0,1,1,0 }  // = 11 (X).
};

byte brightness = 3;

unsigned int currentAddress = 0;
unsigned int currentMemValue = 0;
unsigned int changedValue = 0;
byte memRead = EEPROM_READ_FAILURE;
bool memoryReadfailure = true;
byte tempMemoryEdit = 0;

unsigned char currentMode = 0;
bool modeHex = true;
char editingNumericDigitLight = -1;
bool editingNumericDigitlightState = TM1638_COLOR_NONE;

unsigned char currentScreenValue[SEGMENT_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0 };

byte scanC = 0;
byte scanR = 0;
byte led = 0;
byte col = TM1638_COLOR_NONE;

byte lastKeyPress = NO_KEY;

TM1638 ioMod(DIO, CLK, STB);

void Reset() {
  asm volatile ("  jmp 0");  
}

unsigned char bitsToChar(char *bitArr, unsigned int arrSize) {
  unsigned char res = 0;

  for (unsigned int i = 0; i < arrSize; i++) {
    res <<= 1;
    res += bitArr[i];
  }

  return res;
}

byte getDigit(unsigned int inputNumber, byte loc, byte exponent) {
  if (loc > 0) {
    return (inputNumber / (exponent * loc) % exponent);
  } else if (loc < 0) {
    return 0;
  }
  
  return (inputNumber % exponent);
}

unsigned int setDigit(unsigned int inputNumber, byte newDigit, byte loc, byte exponent) {
  const byte maxSize = 32;

  byte numberSize = (inputNumber == 0 ? 1 : 0);
  unsigned int nSize = inputNumber;
  while (nSize != 0 && numberSize < maxSize) {
    nSize /= exponent;
    numberSize++;
  }
  
  if (loc > maxSize || loc > numberSize || loc < 0) {
    return inputNumber;
  }
  
  char *numberArray = calloc((int)numberSize, sizeof(char));

  unsigned int i = 0;
  unsigned int nCount = inputNumber;
  
  while (nCount != 0 && i < numberSize) {
    numberArray[i] = nCount % exponent;
    nCount /= exponent;
    i++;
  };

  numberArray[loc] = (newDigit % exponent);

  int output = 0;
  
  for (byte j = 0; j < numberSize; j++) {
    output = exponent * output + numberArray[j];
  }
  
  if (numberArray) {
    free(numberArray);
  }
  
  return output;
}

void exEepromWriteByte(int deviceAddress, unsigned int memAddress, byte data) {
  int rdata = data;
  delay(25);
  Wire.beginTransmission(deviceAddress);
  Wire.write((int)(memAddress >> 8)); // MSB
  Wire.write((int)(memAddress & 0xFF)); // LSB
  Wire.write(rdata);
  Wire.endTransmission();
  delay(25);
}

byte exEepromReadByte(int deviceAddress, unsigned int memAddress) {
  byte rdata = EEPROM_READ_FAILURE;
  delay(25);
  Wire.beginTransmission(deviceAddress);
  Wire.write((int)(memAddress >> 8)); // MSB
  Wire.write((int)(memAddress & 0xFF)); // LSB
  Wire.endTransmission();
  Wire.requestFrom(deviceAddress, 1);
  memoryReadfailure = true;
  if (Wire.available()) {
    memoryReadfailure = false;
    rdata = Wire.read();
  }
  delay(25);
  return rdata;
}

byte exEepromReadByte(int deviceAddress, unsigned int memAddress, byte defaultValue) {
  byte rdata = defaultValue;
  delay(25);
  Wire.beginTransmission(deviceAddress);
  Wire.write((int)(memAddress >> 8)); // MSB
  Wire.write((int)(memAddress & 0xFF)); // LSB
  Wire.endTransmission();
  Wire.requestFrom(deviceAddress, 1);
  memoryReadfailure = true;
  if (Wire.available()) {
    memoryReadfailure = false;
    rdata = Wire.read();
  }
  delay(25);
  return rdata;
}

void clearScreen(bool ledsToo) {
  const byte clearDisplay[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    ioMod.setDisplay(clearDisplay);
    if (ledsToo) {
      ioMod.setLEDs(TM1638_COLOR_NONE);
    }
}

unsigned char * breakIntToArray(int inputNumber, bool outputHex) {
  unsigned int modder = 0;
  
  if (outputHex) {
    modder = 16;
  } else {
    modder = 10;
  }

  char *numberArray = calloc(HALF_SEGMENT_COUNT, sizeof(char));

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

byte decodeKeypad() {
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

void setNewAddress(unsigned int newAddress) {
  currentAddress = newAddress & 0xffff;
}

void readAndDisplayMemory() {
  memRead = exEepromReadByte(EEPROM_ADDR, currentAddress, EEPROM_READ_FAILURE);
  unsigned char *memBuffer = breakIntToArray((byte)memRead, modeHex);
  if (memoryReadfailure) {
    updateScreen(displayError, true);
  } else {
    updateScreen(memBuffer, true);
  }
  
  if (memBuffer) {
    free(memBuffer);
  }
}

void setupKeypadMatrixPins() {
  for(byte i = 0; i < pinRows; i++) {
    pinMode(outputPins[i],OUTPUT);
  }
  for(byte j = 0; j < pinCols; j++) {
    pinMode(inputPins[j],INPUT_PULLUP);
  }
}

void updateScreen(char * value, boolean rightSide, bool rawMode) {
  int indexOffset = 0;
  indexOffset = rightSide ? HALF_SEGMENT_COUNT : 0;

  if (rawMode) {
    for (unsigned int i = 0; i < HALF_SEGMENT_COUNT; i++) {
      currentScreenValue[i + indexOffset] = value[i];
    }
  } else {
    for (unsigned int i = 0; i < HALF_SEGMENT_COUNT; i++) {
      currentScreenValue[i + indexOffset] = bitsToChar(sevenSegmentDisplay[value[i]], 8);
    }
  }
}

void updateScreen(char * value, boolean rightSide) {
  updateScreen(value, rightSide, false);
}

void setup() {
  setupKeypadMatrixPins();
  ioMod.setupDisplay(true, brightness);

  Wire.begin();
  Serial.begin(19200);
  
  byte initialDisplay[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  for (byte i = 0, j = 7; i < 8; i++, j--) {
    initialDisplay[i] = { 255 };
    ioMod.setDisplay(initialDisplay);
    ioMod.setLED(TM1638_COLOR_RED, j);
    delay(75);
  }
  
  for (byte i = 0, j = 7; i < 8; i++, j--) {
    initialDisplay[i] = { 0 };
    ioMod.setDisplay(initialDisplay);
    ioMod.setLED(TM1638_COLOR_NONE, j);
    delay(50);
  }
  delay(100);
  byte binDisp[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
  ioMod.setDisplay(binDisp);
  
  delay(500);

  clearScreen(true);
  delay(50);

  currentMode = BASIC_ADDR_INPUT;
  editingNumericDigitLight = -1;
  ioMod.setLED(TM1638_COLOR_RED, 0);
  ioMod.setLED(TM1638_COLOR_RED, 1);
  ioMod.setLED(TM1638_COLOR_RED, 2);
  ioMod.setLED(TM1638_COLOR_RED, 3);
}

void basicAddressInputMode() {
  unsigned char *addrBuffer = breakIntToArray(currentAddress, modeHex);
  
  if (ioMod.getButtons() == 0b00000001) {
    setNewAddress(currentAddress + 1);
  }

  if (ioMod.getButtons() == 0b00000010) {
    setNewAddress(currentAddress - 1);
  }

  if (ioMod.getButtons() == 0b00000100 || ioMod.getButtons() == 0b00001000) {
    currentMode = NUMERIC_ADDR_INPUT;
    editingNumericDigitLight = 3;
    editingNumericDigitlightState = TM1638_COLOR_RED;
  }
  
  if (ioMod.getButtons() == 0b10000000) {
    currentMode = BASIC_MEM_CTRL;
    tempMemoryEdit = memRead;
    ioMod.setLED(TM1638_COLOR_NONE, 0);
    ioMod.setLED(TM1638_COLOR_NONE, 1);
    ioMod.setLED(TM1638_COLOR_NONE, 2);
    ioMod.setLED(TM1638_COLOR_NONE, 3);
    ioMod.setLED(TM1638_COLOR_RED, 4);
    ioMod.setLED(TM1638_COLOR_RED, 5);
    ioMod.setLED(TM1638_COLOR_RED, 6);
    ioMod.setLED(TM1638_COLOR_RED, 7);
  }

  updateScreen(addrBuffer, false);
  if (addrBuffer) {
    free(addrBuffer);
  }
}

void numericAddressInputMode() {
  unsigned char *addrBuffer = breakIntToArray(currentAddress, modeHex);
  
  if (ioMod.getButtons() == 0b00010000) {
    modeHex = !modeHex;
    if (modeHex) {
      updateScreen(displayHex, true);
    } else {
      updateScreen(displayDec, true);
    }
    ioMod.setDisplay(currentScreenValue);
    delay(1000);
    updateScreen(addrBuffer, false);
    readAndDisplayMemory();
  }
  
  if (ioMod.getButtons() == 0b00000001) {
    byte tmpDigit = getDigit(currentAddress, ((HALF_SEGMENT_COUNT - 1) - editingNumericDigitLight), modeHex ? 0xF : 0xA);
    if (tmpDigit == (modeHex ? 0xF : 0xA)) {
      tmpDigit = 0;
    } else {
      tmpDigit++;
    }
    setNewAddress(setDigit(currentAddress, tmpDigit, ((HALF_SEGMENT_COUNT - 1) - editingNumericDigitLight), modeHex ? 0xF : 0xA));
    addrBuffer[editingNumericDigitLight]++;
  }

  if (ioMod.getButtons() == 0b00000010) {
    byte tmpDigit = getDigit(currentAddress, ((HALF_SEGMENT_COUNT - 1) - editingNumericDigitLight), modeHex ? 0xF : 0xA);
    if (tmpDigit == 0) {
      tmpDigit = modeHex ? 0xF : 0xA;
    } else {
      tmpDigit--;
    }
    setNewAddress(setDigit(currentAddress, tmpDigit, ((HALF_SEGMENT_COUNT - 1) - editingNumericDigitLight), modeHex ? 0xF : 0xA));
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
    currentMode = BASIC_ADDR_INPUT;
    editingNumericDigitLight = -1;
    ioMod.setLED(TM1638_COLOR_RED, 0);
    ioMod.setLED(TM1638_COLOR_RED, 1);
    ioMod.setLED(TM1638_COLOR_RED, 2);
    ioMod.setLED(TM1638_COLOR_RED, 3);
  }

  updateScreen(addrBuffer, false);
  if (addrBuffer) {
    free(addrBuffer);
  }
}

void numericMemoryInput() {

  if (ioMod.getButtons() == 0b00000001) {
    tempMemoryEdit = getDigit(tempMemoryEdit, (editingNumericDigitLight - HALF_SEGMENT_COUNT), modeHex ? 0xF : 0xA);
    if (tempMemoryEdit == (modeHex ? 0xF : 0xA)) {
      tempMemoryEdit = 0;
    } else {
      tempMemoryEdit++;
    }
    tempMemoryEdit = setDigit(currentAddress, tempMemoryEdit, (editingNumericDigitLight - HALF_SEGMENT_COUNT), modeHex ? 0xF : 0xA);
    
    unsigned char *tempMemoryEditBuffer = breakIntToArray(tempMemoryEdit, modeHex);
    updateScreen(tempMemoryEditBuffer, true);
    if (tempMemoryEditBuffer) {
      free(tempMemoryEditBuffer);
    }
  }

  if (ioMod.getButtons() == 0b00000010) {
    tempMemoryEdit = getDigit(tempMemoryEdit, (editingNumericDigitLight - HALF_SEGMENT_COUNT), modeHex ? 0xF : 0xA);
    if (tempMemoryEdit == 0) {
      tempMemoryEdit = modeHex ? 0xF : 0xA;
    } else {
      tempMemoryEdit--;
    }
    tempMemoryEdit = setDigit(currentAddress, tempMemoryEdit, (editingNumericDigitLight - HALF_SEGMENT_COUNT), modeHex ? 0xF : 0xA);
    
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
    editingNumericDigitLight = -1;
    ioMod.setLED(TM1638_COLOR_RED, 4);
    ioMod.setLED(TM1638_COLOR_RED, 5);
    ioMod.setLED(TM1638_COLOR_RED, 6);
    ioMod.setLED(TM1638_COLOR_RED, 7);
    currentMode = BASIC_MEM_CTRL;
  }

  if (ioMod.getButtons() == 0b10000000) {    
    exEepromWriteByte(EEPROM_ADDR, currentAddress, tempMemoryEdit);
    editingNumericDigitLight = -1;
    ioMod.setLED(TM1638_COLOR_RED, 4);
    ioMod.setLED(TM1638_COLOR_RED, 5);
    ioMod.setLED(TM1638_COLOR_RED, 6);
    ioMod.setLED(TM1638_COLOR_RED, 7);
    currentMode = BASIC_MEM_CTRL;
  }
}

void basicMemoryInput() {
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
    editingNumericDigitLight = 7;
    currentMode = NUMERIC_MEM_INPUT;
    ioMod.setLED(TM1638_COLOR_RED, 4);
    ioMod.setLED(TM1638_COLOR_RED, 5);
    ioMod.setLED(TM1638_COLOR_RED, 6);
    ioMod.setLED(TM1638_COLOR_RED, 7);
  }
  
  if (ioMod.getButtons() == 0b10000000) {    
    exEepromWriteByte(EEPROM_ADDR, currentAddress, tempMemoryEdit);
    ioMod.setLED(TM1638_COLOR_RED, 4);
    ioMod.setLED(TM1638_COLOR_RED, 5);
    ioMod.setLED(TM1638_COLOR_RED, 6);
    ioMod.setLED(TM1638_COLOR_RED, 7);
    currentMode = BASIC_MEM_CTRL;
  }
  
  if (ioMod.getButtons() == 0b01000000 || ioMod.getButtons() == 0b00100000) {
    tempMemoryEdit = memRead;
    unsigned char *tempMemoryEditBuffer = breakIntToArray(tempMemoryEdit, modeHex);
    updateScreen(tempMemoryEditBuffer, true);
    if (tempMemoryEditBuffer) {
      free(tempMemoryEditBuffer);
    }
    ioMod.setLED(TM1638_COLOR_RED, 4);
    ioMod.setLED(TM1638_COLOR_RED, 5);
    ioMod.setLED(TM1638_COLOR_RED, 6);
    ioMod.setLED(TM1638_COLOR_RED, 7);
    currentMode = BASIC_MEM_CTRL;
  }
}

void basicMemoryControl() {
  if (ioMod.getButtons() == 0b00100000 || ioMod.getButtons() == 0b01000000) {
    currentMode = BASIC_MEM_INPUT;
  }

  if (ioMod.getButtons() == 0b10000000) {
    currentMode = BASIC_ADDR_INPUT;
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

void loop() {
  if (editingNumericDigitLight > -1) {
    if (editingNumericDigitlightState == TM1638_COLOR_RED) {
      editingNumericDigitlightState = TM1638_COLOR_NONE;
    } else {
      editingNumericDigitlightState = TM1638_COLOR_RED;
    }
    ioMod.setLED(editingNumericDigitlightState, editingNumericDigitLight);
  }

  if (currentMode == BASIC_MEM_INPUT) {
    if (editingNumericDigitlightState == TM1638_COLOR_RED) {
      editingNumericDigitlightState = TM1638_COLOR_NONE;
    } else {
      editingNumericDigitlightState = TM1638_COLOR_RED;
    }
    ioMod.setLED(editingNumericDigitlightState, 4);
    ioMod.setLED(editingNumericDigitlightState, 5);
    ioMod.setLED(editingNumericDigitlightState, 6);
    ioMod.setLED(editingNumericDigitlightState, 7);
  }

  delay(REFRESH_SPEED);

  lastKeyPress = decodeKeypad();

  switch (currentMode) {
    case BASIC_ADDR_INPUT: {
      basicAddressInputMode();
      readAndDisplayMemory();
    }
    break;
    case NUMERIC_ADDR_INPUT: {
      numericAddressInputMode();
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
  }

  ioMod.setDisplay(currentScreenValue);

  if (ioMod.getButtons() == 0b00010011) {
    Reset();
  }

//  Serial.print("\n \nScreen Data: ");
//  Serial.print(currentScreenValue[0], HEX);
//  Serial.print(currentScreenValue[1], HEX);
//  Serial.print(currentScreenValue[2], HEX);
//  Serial.print(currentScreenValue[3], HEX);
//  Serial.print(currentScreenValue[4], HEX);
//  Serial.print(currentScreenValue[5], HEX);
//  Serial.print(currentScreenValue[6], HEX);
//  Serial.print(currentScreenValue[7], HEX);
//  Serial.print(currentScreenValue[8], HEX);
//  Serial.print("\nCurrent Address: ");
//  Serial.print(currentAddress, HEX);
//  Serial.print("\nCurrent Memory: ");
//  Serial.print(memRead, HEX);
//  Serial.print("\nCurrent Mode: ");
//  Serial.print(currentMode, HEX);
//  if (lastKeyPress != NO_KEY) {
//    Serial.print("\nLast Keypin Pressed: ");
//    Serial.println(lastKeyPress, HEX);
//  }
//  Serial.println(" ");
}
