#ifndef GENERIC_UTILS_H
#define GENERIC_UTILS_H

void ICACHE_FLASH_ATTR Reset();
boolean ICACHE_FLASH_ATTR stringToNumber(int* res, char inputChar[]);
unsigned char ICACHE_FLASH_ATTR bitsToChar(unsigned char *bitArr, unsigned int arrSize);
boolean isNthBitSet(int bitNumber, int nthBit);
byte ICACHE_FLASH_ATTR hexToInt(char inputHex);
//int ICACHE_FLASH_ATTR stackAvailable();
long ICACHE_FLASH_ATTR heapAvailable();

#endif
