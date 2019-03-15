#include <TM1638.h>

#define STB 2
#define CLK 3
#define DIO 4

#define NO_KEY 0x32

#define SEGMENT_COUNT 8
#define HALF_SEGMENT_COUNT SEGMENT_COUNT / 2

#define BASIC_ADDR_INPUT 0
#define NUMERIC_ADDR_INPUT 1

const byte pinRows = 4;
const byte pinCols = 4;
const byte outputPins[pinRows] = {13, 12, 11, 10};
const byte inputPins[pinCols] = {9, 8, 7, 6};

// Segment labelling:
//        A
//       ----
//     F |  | B
//       ---- G
//     E |  | C
//       ----   .H
//        D

const byte sevenSegmentDisplay[40][8] = {
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
  { 1,1,0,1,0,0,0,0 }  // = r.
};

byte brightness = 3;

unsigned int currentAddress = 0;
unsigned int currentMemValue = 0;
unsigned int changedValue = 0;

unsigned char currentMode = 0;
unsigned bool modeHex = true;
char editingNumericDigitLight = -1;
bool editingNumericDigitlightState = TM1638_COLOR_NONE;

unsigned char currentScreenValue[SEGMENT_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0 };

byte scanC = 0;
byte scanR = 0;
byte led = 0;
byte col = TM1638_COLOR_NONE;

byte lastKeyPress = NO_KEY;

TM1638 ioMod(DIO, CLK, STB);

unsigned char bitsToChar(char *bitArr, unsigned int arrSize) {
  unsigned char res = 0;

  for (unsigned int i = 0; i < arrSize; i++) {
    res <<= 1;
    res += bitArr[i];
  }

  return res;
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
      Serial.println(" ");
      Serial.print("   ");
        Serial.print(currentScreenValue[0 + indexOffset], BIN);
      Serial.println(" ");
  }
}

void updateScreen(char * value, boolean rightSide) {
  updateScreen(value, rightSide, false);
}

void setup() {
  setupKeypadMatrixPins();
  ioMod.setupDisplay(true, brightness);
  byte values[] = { 0b00111111, 2, 4, 8, 16, 32, 64, 255 };

  ioMod.setDisplay(values);
  Serial.begin(19200);
}

void loop() {
  ioMod.setLED(TM1638_COLOR_RED, 0);
  ioMod.setLED(TM1638_COLOR_RED, 1);
  ioMod.setLED(TM1638_COLOR_RED, 2);
  ioMod.setLED(TM1638_COLOR_RED, 3);

  if (editingNumericDigitLight > -1) {
    if (editingNumericDigitlightState == TM1638_COLOR_RED) {
      editingNumericDigitlightState = TM1638_COLOR_NONE;
    } else {
      editingNumericDigitlightState = TM1638_COLOR_RED;
    }
    ioMod.setLED(editingNumericDigitlightState, editingNumericDigitLight);
  }

  delay(175);

  lastKeyPress = decodeKeypad();
  if (lastKeyPress != NO_KEY) {
    Serial.println(lastKeyPress, HEX);
  }

  unsigned char *addrBuffer = breakIntToArray(currentAddress, modeHex);

  switch (currentMode) {
    case BASIC_ADDR_INPUT: {
      if (ioMod.getButtons() == 0b00000001) {
        currentAddress++;
      }
    
      if (ioMod.getButtons() == 0b00000010) {
        currentAddress--;
      }
  
      if (ioMod.getButtons() == 0b00001000) {
        currentMode = NUMERIC_ADDR_INPUT;
        editingNumericDigitLight = 3;
        editingNumericDigitlightState = TM1638_COLOR_RED;
      }
    }
    break;
    case NUMERIC_ADDR_INPUT: {
      if (ioMod.getButtons() == 0b00000001) {
        addrBuffer[editingNumericDigitLight]++;
      }

      if (ioMod.getButtons() == 0b00000010) {
        addrBuffer[editingNumericDigitLight]--;
      }
    
      if (ioMod.getButtons() == 0b00000100) {
        editingNumericDigitLight--;
        if (editingNumericDigitLight < 0) {
          editingNumericDigitLight = 3;
        }
      }
  
      if (ioMod.getButtons() == 0b00001000) {
        currentMode = BASIC_ADDR_INPUT;
        editingNumericDigitLight = -1;
        ioMod.setLED(TM1638_COLOR_RED, 0);
        ioMod.setLED(TM1638_COLOR_RED, 1);
        ioMod.setLED(TM1638_COLOR_RED, 2);
        ioMod.setLED(TM1638_COLOR_RED, 3);
      }
    }
    break;
  }
  
  updateScreen(addrBuffer, false);
  if (addrBuffer) {
    free(addrBuffer);
  }

  ioMod.setDisplay(currentScreenValue);
}
