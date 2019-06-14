#ifndef HTTP_HANDLE_H
#define HTTP_HANDLE_H

void ICACHE_FLASH_ATTR httpHandleNotFound();
void ICACHE_FLASH_ATTR httpHandleInt();
void ICACHE_FLASH_ATTR httpHandleExec();
void ICACHE_FLASH_ATTR httpHandleRoot();
void ICACHE_FLASH_ATTR httpEncodeIntError(const JsonObject &JSONencoder, int locIndex, char* errorChars, char* errorMessage);
void ICACHE_FLASH_ATTR httpEncodeIntError(const JsonObject JSONencoder, char* errorMessage);
void ICACHE_FLASH_ATTR httpSendIntError(char* errorMessage);
void ICACHE_FLASH_ATTR httpSendIntError(int locIndex, char* errorChars, char* errorMessage);

#endif
