#ifndef KEYPAD_HANDLER_H
#define KEYPAD_HANDLER_H

#define NO_KEY 0x32

const byte pinRows = 4;
const byte pinCols = 4;
const byte outputPins[pinRows] = {13, 12, 11, 10};
const byte inputPins[pinCols] = {9, 8, 7, 6};

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

byte ICACHE_FLASH_ATTR decodeKeypad();
void ICACHE_FLASH_ATTR setupKeypadMatrixPins();

#endif
