#ifndef KEYPAD_HANDLER_H
#define KEYPAD_HANDLER_H

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
