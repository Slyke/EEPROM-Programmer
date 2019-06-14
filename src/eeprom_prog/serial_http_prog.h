#ifndef SERIAL_HTTP_PROG_H
#define SERIAL_HTTP_PROG_H

// OP Codes
#define OP_NOP 0x00
#define OP_JMP 0x01
#define OP_AIC 0x02
#define OP_ADC 0x03
#define OP_GETC 0x04
#define OP_GET 0x05
#define OP_MOV 0x06
#define OP_L2M 0x07
#define OP_PI2CC 0xd1
#define OP_SI2CC 0xd2
#define OP_PI2C 0xd3
#define OP_SI2C 0xd4
#define OP_PERC 0xd6
#define OP_PER 0xd7
#define OP_SERC 0xd8
#define OP_SER 0xd9
#define OP_PCPY 0xda
#define OP_INT 0xe0
#define OP_ECH 0xe1
#define OP_ADMP 0xe2
#define OP_CDMP 0xe3
#define OP_RETD 0xf0
#define OP_RET 0xf1
#define OP_I2C 0xf4
#define OP_RST 0xf9
#define STR_OP_NOP "0x00"
#define STR_OP_JMP "0x01"
#define STR_OP_AIC "0x02"
#define STR_OP_ADC "0x03"
#define STR_OP_GETC "0x04"
#define STR_OP_GET "0x05"
#define STR_OP_MOV "0x06"
#define STR_OP_L2M "0x07"
#define STR_OP_PI2CC "0xd1"
#define STR_OP_SI2CC "0xd2"
#define STR_OP_PI2C "0xd3"
#define STR_OP_SI2C "0xd4"
#define STR_OP_PERC "0xd6"
#define STR_OP_PER "0xd7"
#define STR_OP_SERC "0xd8"
#define STR_OP_SER "0xd9"
#define STR_OP_PCPY "0xda"
#define STR_OP_INT "0xe0"
#define STR_OP_ECH "0xe1"
#define STR_OP_ADMP "0xe2"
#define STR_OP_CDMP "0xe3"
#define STR_OP_RETD "0xf0"
#define STR_OP_RET "0xf1"
#define STR_OP_I2C "0xf4"
#define STR_OP_RST "0xf9"

#define MAX_OP_PARAM_LENGTH 2
#define DEFAULT_MESSAGE_SIZE 34

void ICACHE_FLASH_ATTR processSerialInput(char resMessage[], char errorMessage[], char echoMessage[]);
void ICACHE_FLASH_ATTR processCommandInputFromSerial(char* commSeg, char resMessage[], char errorMessage[], char echoMessage[]);
void ICACHE_FLASH_ATTR processCommandInputFromSerial(char* commSeg, bool assemblyMode, char resMessage[], char errorMessage[], char echoMessage[]);
void ICACHE_FLASH_ATTR processCommandInputByteCode(char* byteCodeArr, int byteCodeLength, char resMessage[], char errorMessage[], char echoMessage[]);
void ICACHE_FLASH_ATTR commandParamParse(char opByteArray[], int paramPos, unsigned int params[], byte paramsLength);
void ICACHE_FLASH_ATTR execInputCommands(byte command, unsigned int *params, byte paramsLength, char resMessage[], char errorMessage[], char echoMessage[], int resMessageLength, int errorMessageLength, int echoMessageLength);
void ICACHE_FLASH_ATTR execInputCommands(byte command, unsigned int *params, byte paramsLength, char resMessage[], char errorMessage[], char echoMessage[]);
void ICACHE_FLASH_ATTR commandToOpAndParam(char currentToken, byte ret[]);
void ICACHE_FLASH_ATTR processCommandInputHttp(char* byteStream, int byteStreamLength, char **resMessage, char errorMessage[], char **echoMessage, int* commandCount);

boolean ICACHE_FLASH_ATTR commandDecode(byte ret[], char currentToken[], byte remainingLength, char errorMessage[]);


#endif
