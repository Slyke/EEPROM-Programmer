
void ICACHE_FLASH_ATTR updateScreenConnect(int currentTry) {
  for (byte i = 0, j = 7; i < 8; i++, j--) {
    ioMod.setLED(TM1638_COLOR_NONE, j);
  }

  ioMod.setLED(TM1638_COLOR_RED, (currentTry % 8));
}

void ICACHE_FLASH_ATTR updateScreen(unsigned char * value, boolean rightSide) {
  updateScreen(value, rightSide, false);
}

void ICACHE_FLASH_ATTR readAndDisplayAddress() {
  unsigned char *addrBuffer = (unsigned char*)calloc(HALF_SEGMENT_COUNT, sizeof(char));
  breakIntToArray(addrBuffer, currentAddress, modeHex);

  updateScreen(addrBuffer, false);
  
  if (addrBuffer) {
    free(addrBuffer);
  }
}

void ICACHE_FLASH_ATTR readAndDisplayMemory() {
  const byte readRes = exEepromReadByte(&memRead, primaryEepromReader, currentAddress, EEPROM_READ_FAILURE);
  
  unsigned char *memBuffer = (unsigned char*)calloc(HALF_SEGMENT_COUNT, sizeof(char));
  breakIntToArray(memBuffer, (byte)memRead, modeHex);

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

void ICACHE_FLASH_ATTR clearScreen(bool ledsToo) {
  const byte clearDisplay[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  ioMod.setDisplay(clearDisplay);
  if (ledsToo) {
    ioMod.setLEDs(TM1638_COLOR_NONE);
  }
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
