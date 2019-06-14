
void ICACHE_FLASH_ATTR Reset() {
  #ifdef IS_ATMEG
    asm volatile ("  jmp 0");
  #else
    ESP.reset();
  #endif
}

boolean ICACHE_FLASH_ATTR stringToNumber(int* res, char inputChar[]) {
  String strInputChar(inputChar);
  char *endPtr = NULL;
  if (strInputChar.indexOf(F("0x")) > -1) {
    *res = (int)strtol(inputChar, &endPtr, 16) % 0xFFFF;
  } else if (strInputChar.indexOf(F("0b")) > -1) {
    *res = (int)strtol(inputChar, &endPtr, 2) % 0xFFFF;
  } else {
    *res = (int)strtol(inputChar, &endPtr, 10) % 0xFFFF;
  }
  
  if (inputChar && !*endPtr) {
    return true;
  }
  return false;
}

unsigned char ICACHE_FLASH_ATTR bitsToChar(char *bitArr, unsigned int arrSize) {
  unsigned char res = 0;

  for (unsigned int i = 0; i < arrSize; i++) {
    res <<= 1;
    res += bitArr[i];
  }

  return res;
}

byte ICACHE_FLASH_ATTR hexToInt(char inputHex) {
  byte res = 0;

  if(inputHex <= 57) {
    res = inputHex - '0';
  } else {
    res = inputHex - 'A' + 10;
  }

  return res;
}

extern "C"
{
#include <cont.h>
  extern cont_t* g_pcont;
  int ICACHE_FLASH_ATTR stackAvailable() {
    register uint32_t *sp asm("a1");
    return 4 * (sp - g_pcont->stack);
  }
}

long ICACHE_FLASH_ATTR heapAvailable() {
  return ESP.getFreeHeap();
}
