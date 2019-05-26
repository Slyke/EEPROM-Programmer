#ifndef GENERIC_UTILS_H
#define GENERIC_UTILS_H

void ICACHE_FLASH_ATTR Reset();
boolean ICACHE_FLASH_ATTR stringToNumber(int* res, char inputChar[]);
unsigned char ICACHE_FLASH_ATTR bitsToChar(char *bitArr, unsigned int arrSize);
byte ICACHE_FLASH_ATTR hexToInt(char inputHex);

#endif
