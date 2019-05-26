#ifndef SCREEN_HANDLE_H
#define SCREEN_HANDLE_H

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

unsigned int ICACHE_FLASH_ATTR setDigit(unsigned int inputNumber, byte newDigit, byte pos, byte radix);
byte ICACHE_FLASH_ATTR getDigit(unsigned int inputNumber, byte loc, byte radix);
void ICACHE_FLASH_ATTR clearScreen(bool ledsToo);
void ICACHE_FLASH_ATTR updateScreen(unsigned char * value, boolean rightSide);
void ICACHE_FLASH_ATTR updateScreen(unsigned char * value, boolean rightSide, bool rawMode);
void ICACHE_FLASH_ATTR readAndDisplayMemory();
void ICACHE_FLASH_ATTR readAndDisplayAddress();
void ICACHE_FLASH_ATTR updateScreenConnect(int currentTry);

#endif
