#ifndef KEYPAD_HANDLER_H
#define KEYPAD_HANDLER_H

#define NO_KEY 0x32

const byte pinRows = 4;
const byte pinCols = 4;
const byte outputPins[pinRows] = {4, 5, 6, 7};
const byte inputPins[pinCols] = {0, 1, 2, 3};

const byte keyMap[16] = {
  13,
  15,
  0,
  14,
  12,
  9,
  8,
  7,
  11,
  6,
  5,
  4,
  10,
  3,
  2,
  1
};

const unsigned int ICACHE_FLASH_ATTR getKeyStates();
byte ICACHE_FLASH_ATTR decodeKeypad(const unsigned int keypadState);

#endif
