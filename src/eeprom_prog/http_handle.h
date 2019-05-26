#ifndef HTTP_HANDLE_H
#define HTTP_HANDLE_H

void ICACHE_FLASH_ATTR httpHandleNotFound();
void ICACHE_FLASH_ATTR httpHandleInt();
void ICACHE_FLASH_ATTR httpHandleIntError(int locIndex, char* errorChars, char* errorMessage);
void ICACHE_FLASH_ATTR httpHandleRoot();

#endif
