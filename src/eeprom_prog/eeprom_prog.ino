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
#define SERIAL_COMMAND 5

#define DEFAULT_RUN_MODE BASIC_ADDR_INPUT

#define EEPROM_ADDR 0x50 // I2C Address
#define EEPROM_READ_FAILURE 0xFF // Returned value on fail. Don't set to 0, 1 or EE_PROM_VERSION

#define COMM_SPEED 19200
#define REFRESH_SPEED 150

// OP Codes
#define OP_NOP 0x00
#define OP_JMP 0x01
#define OP_AIC 0x02
#define OP_ADC 0x03
#define OP_GETC 0x04
#define OP_GET 0x05
#define OP_MOV 0x06
#define OP_L2M 0x07
#define OP_INT 0xe0
#define OP_ECH 0xe1
#define OP_RETD 0xf0
#define OP_RET 0xf1
#define OP_RST 0xf9
#define STR_OP_NOP "0x00"
#define STR_OP_JMP "0x01"
#define STR_OP_AIC "0x02"
#define STR_OP_ADC "0x03"
#define STR_OP_GETC "0x04"
#define STR_OP_GET "0x05"
#define STR_OP_MOV "0x06"
#define STR_OP_L2M "0x07"
#define STR_OP_INT "0xe0"
#define STR_OP_ECH "0xe1"
#define STR_OP_RETD "0xf0"
#define STR_OP_RET "0xf1"
#define STR_OP_RST "0xf9"

const byte pinRows = 4;
const byte pinCols = 4;
const byte outputPins[pinRows] = {13, 12, 11, 10};
const byte inputPins[pinCols] = {9, 8, 7, 6};
const unsigned char displayError[4] = {14, 38, 40, 38};
const unsigned char displayDec[4] = {16, 13, 14, 12};
const unsigned char displayHex[4] = {16, 42, 14, 44};
const unsigned char displayEcho[4] = {14, 12, 42, 0};
const unsigned char displayOn[4] = {16, 16, 0, 1};
const unsigned char displayOff[4] = {16, 0, 15, 15};

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
bool memoryReadfailure = true;
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

byte hexToInt(char inputHex) {
  byte res = 0;

  if(inputHex <= 57) {
    res = inputHex - '0';
  } else {
    res = inputHex - 'A' + 10;
  }

  return res;
}

byte getDigit(unsigned int inputNumber, byte loc, byte radix) {
  int res = 0;

  byte numBuff[7] = {0};
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

unsigned int setDigit(unsigned int inputNumber, byte newDigit, byte pos, byte radix) {
  unsigned int posValue = 1;
  while (pos > 0) {
    posValue *= radix;
    --pos;
  }

  return inputNumber + ((int)newDigit - (int)((inputNumber / posValue) % radix)) * posValue;
}

void exEepromWriteByte(int deviceAddress, unsigned int memAddress, byte data) {
  int rdata = data;
  delay(25);
  Wire.beginTransmission(deviceAddress);
  Wire.write((int)(memAddress >> 8)); // MSB
  Wire.write((int)(memAddress & 0xFF)); // LSB
  Wire.write(rdata & 0xFF);
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

void readAndDisplayAddress() {
  unsigned char *addrBuffer = breakIntToArray(currentAddress, modeHex);
  updateScreen(addrBuffer, false);
  
  if (addrBuffer) {
    free(addrBuffer);
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
  Serial.begin(COMM_SPEED);

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
  delay(25);
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
  delay(20);

  currentMode = DEFAULT_RUN_MODE;
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

  if (ioMod.getButtons() == 0b00010000) {
    currentMode = SERIAL_COMMAND;
    Serial.println("Serial Listening...");
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
  
  if (lastKeyPress != NO_KEY) {
    const byte editingDigit = ((HALF_SEGMENT_COUNT - 1) - editingNumericDigitLight);
    unsigned int tmpDigit = keyMap[lastKeyPress];

    setNewAddress(setDigit(currentAddress, tmpDigit, ((HALF_SEGMENT_COUNT - 1) - editingNumericDigitLight), modeHex ? 0x10 : 10));
    addrBuffer[editingNumericDigitLight] = tmpDigit;
  }

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
  if (editingNumericDigitlightState == TM1638_COLOR_RED) {
    editingNumericDigitlightState = TM1638_COLOR_NONE;
  } else {
    editingNumericDigitlightState = TM1638_COLOR_RED;
  }
  ioMod.setLED(editingNumericDigitlightState, 4);
  ioMod.setLED(editingNumericDigitlightState, 5);
  ioMod.setLED(editingNumericDigitlightState, 6);
  ioMod.setLED(editingNumericDigitlightState, 7);
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
      updateScreen(displayHex, true);
    } else {
      updateScreen(displayDec, true);
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
  
  if (ioMod.getButtons() == 0b00010000) {
    modeHex = !modeHex;
    if (modeHex) {
      updateScreen(displayHex, true);
    } else {
      updateScreen(displayDec, true);
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

  if (ioMod.getButtons() == 0b00010000) {
    currentMode = SERIAL_COMMAND;
    Serial.println("Serial Listening...");
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
        memRead = exEepromReadByte(EEPROM_ADDR, params[0], EEPROM_READ_FAILURE);
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
        exEepromWriteByte(EEPROM_ADDR, currentAddress, params[1] & 0xFF);
        memRead = exEepromReadByte(EEPROM_ADDR, currentAddress, EEPROM_READ_FAILURE);
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
        exEepromWriteByte(EEPROM_ADDR, currentAddress, params[0] & 0xFF);
        memRead = exEepromReadByte(EEPROM_ADDR, currentAddress, EEPROM_READ_FAILURE);
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
}

void commandDecode(byte ret[], char currentToken[], byte paramsLength, unsigned int params[], char remaining[]) {
  String strRemaining(remaining);
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
  } else if (strcmp(currentToken, STR_OP_INT) == 0) {
    ret[0] = OP_INT;
    ret[1] = 1;
  } else if (strcmp(currentToken, STR_OP_ECH) == 0) {
    ret[0] = OP_ECH;
    ret[1] = 1;
  } else if (strcmp(currentToken, STR_OP_RETD) == 0) {
    ret[0] = OP_RETD;
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
    if (strRemaining.length() == 0) {
      ret[0] = OP_GETC;
      ret[1] = 0;
    }
  } else if (strcmp(currentToken, "mov") == 0) {
    ret[0] = OP_MOV;
    ret[1] = 2;
  } else if (strcmp(currentToken, "l2m") == 0) {
    ret[0] = OP_L2M;
    ret[1] = 1;
  } else if (strcmp(currentToken, "int") == 0) {
    ret[0] = OP_INT;
    ret[1] = 1;
  } else if (strcmp(currentToken, "ech") == 0) {
    ret[0] = OP_ECH;
    ret[1] = 1;
  } else if (strcmp(currentToken, "ret") == 0) {
    ret[0] = OP_RET;
    ret[1] = 1;
    if (strRemaining.length() == 0) {
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

void commandParamParse(char currentToken[], unsigned int params[], byte &paramsLength) {
  String strCurrentToken(currentToken);
  if (paramsLength < 2) { // Ignore extras
    if (strCurrentToken.indexOf(F("0x")) > -1) {
      params[paramsLength] = (int)strtol(currentToken, 0, 16) % 0xFFFF;
    } else if (strCurrentToken.indexOf(F("0b")) > -1) {
      params[paramsLength] = (int)strtol(currentToken, 0, 2) % 0xFFFF;
    } else {
      params[paramsLength] = (int)strtol(currentToken, 0, 10) % 0xFFFF;
    }
    paramsLength++;
  }
}

void serialCommandInput() {
  if (!Serial) {
    currentMode = BASIC_ADDR_INPUT;
    return ;
  }

  if (ioMod.getButtons() == 0b10000000 || ioMod.getButtons() == 0b01000000 || ioMod.getButtons() == 0b00100000 || ioMod.getButtons() == 0b00000001 || ioMod.getButtons() == 0b00000010) {
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

  if (Serial.available()) {
    String serialResponse = Serial.readStringUntil('\r\n');
    char *commSeg = serialResponse.c_str();
    char *remaining;
    char *currentToken;
    
    byte paramsLength = 0;
    byte commandDecodeRet[2] = {0};
    unsigned int params[2] = {0};

    if (assemblyInterpretMode) {
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
            commandParamParse(currentToken, params, paramsLength);
            if (strRemaining.length() == 0) {
              parseSerialCommands(commandDecodeRet[0], params, paramsLength);
            }
          } else {
            // Command decoder
            commandDecode(commandDecodeRet, currentToken, paramsLength, params, remaining);
  
            if (strRemaining.length() == 0) {
              parseSerialCommands(commandDecodeRet[0], params, paramsLength);
            }
          }
        }
      }
    } else {
      remaining = commSeg;
      while ((currentToken = strtok_r(remaining, " ", &remaining)) != NULL) {

        String strRemaining(remaining);
        if (commandDecodeRet[0] > 0) {
          // Command parameter parser
          commandParamParse(currentToken, params, paramsLength);
 
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
          commandDecode(commandDecodeRet, currentToken, paramsLength, params, remaining);

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
    }
  }
  
  if (editingNumericDigitlightState == TM1638_COLOR_RED) {
    editingNumericDigitlightState = TM1638_COLOR_NONE;
  } else {
    editingNumericDigitlightState = TM1638_COLOR_RED;
  }
  ioMod.setLED(editingNumericDigitlightState, 0);
  ioMod.setLED(editingNumericDigitlightState, 1);
  ioMod.setLED(editingNumericDigitlightState, 2);
  ioMod.setLED(editingNumericDigitlightState, 3);
  ioMod.setLED(editingNumericDigitlightState, 4);
  ioMod.setLED(editingNumericDigitlightState, 5);
  ioMod.setLED(editingNumericDigitlightState, 6);
  ioMod.setLED(editingNumericDigitlightState, 7);
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
    case SERIAL_COMMAND: {
      serialCommandInput();
      readAndDisplayMemory();
    }
    break;
    
    default: {
      currentMode = DEFAULT_RUN_MODE;
    }
  }

  ioMod.setDisplay(currentScreenValue);

  if (ioMod.getButtons() == 0b00010011) {
    Reset();
  }

  if (ioMod.getButtons() == 0b11010000) {
    updateScreen(displayEcho, false);
    if (serialEchoCommand) {
      updateScreen(displayOff, true);
      serialEchoCommand = false;
    } else {
      updateScreen(displayOn, true);
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
