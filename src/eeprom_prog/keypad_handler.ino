
const unsigned int ICACHE_FLASH_ATTR getKeyStates() {
  unsigned int res = 0;
  for(byte r = 0; r < pinCols; r++) {
    PCF_20.write(outputPins[r], LOW);
    for(byte c = 0; c < pinRows; c++) {
      if (PCF_20.read(inputPins[c]) == LOW) {
        res += (1 << (c + (r * 4)));
      }
    }
    PCF_20.write(outputPins[r], HIGH);
  }

  return res;
}

byte ICACHE_FLASH_ATTR decodeKeypad(const unsigned int keypadState) {
  if (keypadState != 0) {
    for (byte i = 1; i <= 16; i++) {
      if (isNthBitSet(keypadState, i)) {
        return i - 1;
      }
    }
  }
  return NO_KEY;
}
