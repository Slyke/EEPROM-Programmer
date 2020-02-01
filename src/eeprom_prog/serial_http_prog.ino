
void ICACHE_FLASH_ATTR processCommandInputFromSerial(char* commSeg, char resMessage[], char errorMessage[], char echoMessage[], int resMessageLength, int errorMessageLength, int echoMessageLength) {
  processCommandInputFromSerial(commSeg, assemblyInterpretMode, resMessage, errorMessage, echoMessage, resMessageLength, errorMessageLength, echoMessageLength);
}

void ICACHE_FLASH_ATTR processSerialInput(char resMessage[], char errorMessage[], char echoMessage[], int resMessageLength, int errorMessageLength, int echoMessageLength) {
  if (Serial.available()) {
    String serialResponse = Serial.readStringUntil('\n');
    serialResponse.trim();
    char *commSeg = (char *)serialResponse.c_str();
    processCommandInputFromSerial(commSeg, resMessage, errorMessage, echoMessage, resMessageLength, errorMessageLength, echoMessageLength); 
  }
}

void ICACHE_FLASH_ATTR processCommandInputFromSerial(char* commSeg, bool assemblyMode, char resMessage[], char errorMessage[], char echoMessage[], int resMessageLength, int errorMessageLength, int echoMessageLength) {
  char *remaining;
  char *currentToken;
  
  byte paramsLength = 0;
  byte commandDecodeRet[2] = {0};
  int params[MAX_OP_PARAM_LENGTH] = {0};

  if (assemblyMode) {
    while ((remaining = strtok_r(commSeg, ";", &commSeg)) != NULL) {
      memset(commandDecodeRet, 0, sizeof(commandDecodeRet));
      paramsLength = 0;
      memset(params, 0, sizeof(params));
      char *commandOp = strtok_r(remaining, " ", &remaining);
      commandDecode(commandDecodeRet, commandOp, remaining != NULL ? strlen(remaining) : 0, errorMessage);
      
      while ((currentToken = strtok_r(remaining, " ", &remaining)) != NULL && paramsLength < OP_PROCESS_ARR_LIMIT && paramsLength < commandDecodeRet[1]) {
        int res;
        stringToNumber((int *)&res, currentToken);
        params[paramsLength] = res;
        paramsLength++;
      }

      char byteArray[paramsLength + 1];
      byteArray[0] = commandDecodeRet[0];

      for (int i = 0; i < MAX_OP_PARAM_LENGTH && i < paramsLength; i++) {
        byteArray[i + 1] = params[i];
      }
      processCommandInputByteCode(byteArray, paramsLength + 1, resMessage, errorMessage, echoMessage, resMessageLength, errorMessageLength, echoMessageLength);
    }
  } else {
    remaining = commSeg;
    int locationIndex = 0;
    byte byteStream[OP_PROCESS_ARR_LIMIT];
    while ((currentToken = strtok_r(remaining, " ", &remaining)) != NULL && locationIndex < OP_PROCESS_ARR_LIMIT) {
      int res;
      stringToNumber((int *)&res, currentToken);
      byteStream[locationIndex] = (byte)res;
      locationIndex++;
    }

    for (int i = 0; i < OP_PROCESS_ARR_LIMIT && i < locationIndex; i++) {
      commandToOpAndParam((char)byteStream[i], commandDecodeRet);
      unsigned int params[MAX_OP_PARAM_LENGTH] = {0};
      
      for (int j = 0; j < MAX_OP_PARAM_LENGTH && j < (commandDecodeRet[1]); j++) {
        params[j] = byteStream[j + 1];
      }

      execInputCommands(commandDecodeRet[0], params, commandDecodeRet[1], resMessage, errorMessage, echoMessage, resMessageLength, errorMessageLength, echoMessageLength);

      i += commandDecodeRet[1];
    }
  }
}

void ICACHE_FLASH_ATTR processCommandInputByteCode(char* byteCodeArr, int byteCodeLength, char resMessage[], char errorMessage[], char echoMessage[], int resMessageLength, int errorMessageLength, int echoMessageLength) {
  char *remaining;
  byte paramsLength = 0;
  byte commandDecodeRet[2] = {0};
  unsigned int params[MAX_OP_PARAM_LENGTH] = {0};

  for (int i = 0; i < byteCodeLength && i < OP_PROCESS_ARR_LIMIT; i++) {
    if (commandDecodeRet[0] > 0) {
      // Command parameter parser
      commandParamParse(byteCodeArr, i, params, paramsLength);

      if (paramsLength >= commandDecodeRet[1]) {
        execInputCommands(commandDecodeRet[0], params, paramsLength, resMessage, errorMessage, echoMessage, resMessageLength, errorMessageLength, echoMessageLength);
        i += paramsLength;

        memset(params, 0, sizeof(params) * MAX_OP_PARAM_LENGTH);
        memset(commandDecodeRet, 0, sizeof(commandDecodeRet) * 2);
        paramsLength = 0;
      }
    } else {
      // Command decoder
      commandDecodeRet[0] = 0;
      paramsLength = 0;
      memset(params, 0, sizeof(params) * MAX_OP_PARAM_LENGTH);
      memset(commandDecodeRet, 0, sizeof(commandDecodeRet) * 2);
      
      commandToOpAndParam(byteCodeArr[i], commandDecodeRet);
      paramsLength = commandDecodeRet[1];
      if (paramsLength == 0) {
        execInputCommands(commandDecodeRet[0], params, paramsLength, resMessage, errorMessage, echoMessage, resMessageLength, errorMessageLength, echoMessageLength);
      } else {
        if (byteCodeLength <= paramsLength) {
          // This should not execute the OP and instead place an error in errorMessage.
          execInputCommands(commandDecodeRet[0], params, byteCodeLength - 1, resMessage, errorMessage, echoMessage, resMessageLength, errorMessageLength, echoMessageLength);
          break;
        }
      }
    }
  }
}

void ICACHE_FLASH_ATTR processCommandInputFromHttp(char* commStr, char* paramStr1, char* paramStr2, byte paramsLength, char resMessage[], char errorMessage[], char echoMessage[], int resMessageLength, int errorMessageLength, int echoMessageLength) {
  byte commandDecodeRet[2] = {0};
  unsigned int params[MAX_OP_PARAM_LENGTH] = {0};
  int paramCount = 0;

  if (paramsLength > 0 && paramStr1 != NULL) {
    stringToNumber((int *)&params[0], paramStr1);
    paramCount++;
    if (paramsLength > 1 && paramStr2 != NULL) {
      paramCount++;
      stringToNumber((int *)&params[1], paramStr2);
    }
  }
  
  commandDecode(commandDecodeRet, commStr, paramsLength, errorMessage);

  execInputCommands(commandDecodeRet[0], params, paramCount, resMessage, errorMessage, echoMessage, resMessageLength, errorMessageLength, echoMessageLength);
}

void ICACHE_FLASH_ATTR processCommandInputJson(char* byteStream, int byteStreamLength, char **resMessage, char errorMessage[], char **echoMessage, int* commandCount) {
  *commandCount = 0;

  for (int i = 0; i < OP_PROCESS_ARR_LIMIT && i < byteStreamLength; i++) {
    byte commandDecodeRet[2] = {0};
    unsigned int params[MAX_OP_PARAM_LENGTH] = {0};

    commandToOpAndParam(byteStream[i], commandDecodeRet);

    for (int j = 0; j < MAX_OP_PARAM_LENGTH && j < (commandDecodeRet[1]); j++) {
      params[j] = byteStream[j + 1];
    }

    int allocateSize = DEFAULT_MESSAGE_SIZE;

    if (commandDecodeRet[0] == 0xe2) {
      allocateSize = params[0] < OP_PROCESS_ARR_LIMIT ? params[0] : OP_PROCESS_ARR_LIMIT;
    }
    
    if (commandDecodeRet[0] == 0xe3) {
      allocateSize = params[1] < OP_PROCESS_ARR_LIMIT ? params[1] : OP_PROCESS_ARR_LIMIT;
    }

    resMessage[*commandCount] = (char *) calloc(allocateSize, sizeof(char));
    echoMessage[*commandCount] = (char *) calloc(DEFAULT_MESSAGE_SIZE, sizeof(char));

    if (resMessage[*commandCount] == NULL) {
      strcpy(errorMessage, "RAM alloc for *execRes fail");
      break;
    }
    
    if (echoMessage[*commandCount] == NULL) {
      strcpy(errorMessage, "RAM alloc for *echoBack fail");
      break;
    }
    
    execInputCommands(commandDecodeRet[0], params, commandDecodeRet[1], resMessage[*commandCount], errorMessage, echoMessage[*commandCount], allocateSize, DEFAULT_MESSAGE_SIZE, DEFAULT_MESSAGE_SIZE);
    i += commandDecodeRet[1];
    (*commandCount)++;
  }
}

void ICACHE_FLASH_ATTR commandParamParse(char opByteArray[], int paramPos, unsigned int params[], byte paramsLength) {
  for (byte i = 0; i < paramsLength && i < MAX_OP_PARAM_LENGTH; i++) {
    params[i] = opByteArray[i + paramPos];
  }
}

void ICACHE_FLASH_ATTR commandToOpAndParam(char currentToken, byte ret[]) {
  ret[0] = currentToken;
  ret[1] = 0;
  
  switch(currentToken) {
    case OP_JMP: case OP_GETC: case OP_L2M: case OP_PI2CC: case OP_SI2CC: case OP_PERC: case OP_SERC: case OP_INT: case OP_ECH: case OP_CDMP: case OP_RET: case OP_DNUM:
      ret[1] = 1;
      break;
    
    case OP_MOV: case OP_PCPY: case OP_ADMP:
      ret[1] = 2;
      break;
  }
}

boolean ICACHE_FLASH_ATTR commandDecode(byte ret[], char currentToken[], byte remainingLength, char errorMessage[]) {
  ret[0] = OP_NOP;
  ret[1] = 0;

  if (strcmp(currentToken, STR_OP_NOP) == 0) {
    commandToOpAndParam(OP_NOP, ret);
  } else if (strcmp(currentToken, STR_OP_JMP) == 0) {
    commandToOpAndParam(OP_JMP, ret);
  } else if (strcmp(currentToken, STR_OP_AIC) == 0) {
    commandToOpAndParam(OP_AIC, ret);
  } else if (strcmp(currentToken, STR_OP_ADC) == 0) {
    commandToOpAndParam(OP_ADC, ret);
  } else if (strcmp(currentToken, STR_OP_GETC) == 0) {
    commandToOpAndParam(OP_GETC, ret);
  } else if (strcmp(currentToken, STR_OP_GET) == 0) {
    commandToOpAndParam(OP_GET, ret);
  } else if (strcmp(currentToken, STR_OP_MOV) == 0) {
    commandToOpAndParam(OP_MOV, ret);
  } else if (strcmp(currentToken, STR_OP_L2M) == 0) {
    commandToOpAndParam(OP_L2M, ret);
  } else if (strcmp(currentToken, STR_OP_PI2CC) == 0) {
    commandToOpAndParam(OP_PI2CC, ret);
  } else if (strcmp(currentToken, STR_OP_SI2CC) == 0) {
    commandToOpAndParam(OP_SI2CC, ret);
  } else if (strcmp(currentToken, STR_OP_PI2C) == 0) {
    commandToOpAndParam(OP_PI2C, ret);
  } else if (strcmp(currentToken, STR_OP_SI2C) == 0) {
    commandToOpAndParam(OP_SI2C, ret);
  } else if (strcmp(currentToken, STR_OP_PERC) == 0) {
    commandToOpAndParam(OP_PERC, ret);
  } else if (strcmp(currentToken, STR_OP_PER) == 0) {
    commandToOpAndParam(OP_PER, ret);
  } else if (strcmp(currentToken, STR_OP_SERC) == 0) {
    commandToOpAndParam(OP_SERC, ret);
  } else if (strcmp(currentToken, STR_OP_SER) == 0) {
    commandToOpAndParam(OP_SER, ret);
  } else if (strcmp(currentToken, STR_OP_PCPY) == 0) {
    commandToOpAndParam(OP_PCPY, ret);
  } else if (strcmp(currentToken, STR_OP_INT) == 0) {
    commandToOpAndParam(OP_INT, ret);
  } else if (strcmp(currentToken, STR_OP_ECH) == 0) {
    commandToOpAndParam(OP_ECH, ret);
  } else if (strcmp(currentToken, STR_OP_RETD) == 0) {
    commandToOpAndParam(OP_RETD, ret);
  } else if (strcmp(currentToken, STR_OP_ADMP) == 0) {
    commandToOpAndParam(OP_ADMP, ret);
  } else if (strcmp(currentToken, STR_OP_CDMP) == 0) {
    commandToOpAndParam(OP_CDMP, ret);
  } else if (strcmp(currentToken, STR_OP_I2C) == 0) {
    commandToOpAndParam(OP_I2C, ret);
  } else if (strcmp(currentToken, STR_OP_RET) == 0) {
    commandToOpAndParam(OP_RET, ret);
  } else if (strcmp(currentToken, STR_OP_RST) == 0) {
    commandToOpAndParam(OP_RST, ret);
  } else if (strcmp(currentToken, STR_OP_STAT) == 0) {
    commandToOpAndParam(OP_STAT, ret);
  } else if (strcmp(currentToken, STR_OP_MEM) == 0) {
    commandToOpAndParam(OP_MEM, ret);
  } else if (strcmp(currentToken, STR_OP_DNUM) == 0) {
    commandToOpAndParam(OP_DNUM, ret);
  } else if (strcmp(currentToken, "nop") == 0) {
    commandToOpAndParam(OP_NOP, ret);
  } else if (strcmp(currentToken, "jmp") == 0) {
    commandToOpAndParam(OP_JMP, ret);
  } else if (strcmp(currentToken, "aic") == 0 || strcmp(currentToken, "inc") == 0) {
    commandToOpAndParam(OP_AIC, ret);
  } else if (strcmp(currentToken, "adc") == 0 || strcmp(currentToken, "dec") == 0) {
    commandToOpAndParam(OP_ADC, ret);
  } else if (strcmp(currentToken, "get") == 0) {
    commandToOpAndParam(OP_GETC, ret);
    if (remainingLength == 0) {
      commandToOpAndParam(OP_GET, ret);
    }
  } else if (strcmp(currentToken, "mov") == 0) {
    commandToOpAndParam(OP_MOV, ret);
  } else if (strcmp(currentToken, "l2m") == 0) {
    commandToOpAndParam(OP_L2M, ret);
  } else if (strcmp(currentToken, "pi2c") == 0) {
    commandToOpAndParam(OP_PI2C, ret);
    if (remainingLength == 0) {
      commandToOpAndParam(OP_PI2CC, ret);
    }
  } else if (strcmp(currentToken, "si2c") == 0) {
    commandToOpAndParam(OP_SI2C, ret);
    if (remainingLength == 0) {
      commandToOpAndParam(OP_SI2CC, ret);
    }
  } else if (strcmp(currentToken, "per") == 0) {
    commandToOpAndParam(OP_PER, ret);
    if (remainingLength == 0) {
      commandToOpAndParam(OP_PERC, ret);
    }
  } else if (strcmp(currentToken, "ser") == 0) {
    commandToOpAndParam(OP_SER, ret);
    if (remainingLength == 0) {
      commandToOpAndParam(OP_SERC, ret);
    }
  } else if (strcmp(currentToken, "pcpy") == 0) {
    commandToOpAndParam(OP_PCPY, ret);
  } else if (strcmp(currentToken, "int") == 0) {
    commandToOpAndParam(OP_INT, ret);
  } else if (strcmp(currentToken, "ech") == 0) {
    commandToOpAndParam(OP_ECH, ret);
  } else if (strcmp(currentToken, "admp") == 0) {
    commandToOpAndParam(OP_ADMP, ret);
  } else if (strcmp(currentToken, "cdmp") == 0) {
    commandToOpAndParam(OP_CDMP, ret);
  } else if (strcmp(currentToken, "i2c") == 0) {
    commandToOpAndParam(OP_I2C, ret);
  } else if (strcmp(currentToken, "mem") == 0) {
    commandToOpAndParam(OP_MEM, ret);
  } else if (strcmp(currentToken, "dnum") == 0) {
    commandToOpAndParam(OP_DNUM, ret);
  } else if (strcmp(currentToken, "ret") == 0) {
    commandToOpAndParam(OP_RET, ret);
    if (remainingLength == 0) {
      commandToOpAndParam(OP_RETD, ret);
    }
  } else if (strcmp(currentToken, "rst") == 0) {
    commandToOpAndParam(OP_RST, ret);
  } else if (strcmp(currentToken, "stat") == 0) {
    commandToOpAndParam(OP_STAT, ret);
  } else {
    char outputBuf[32];
    String strCurrentToken(currentToken);
    sprintf(outputBuf, "0x%02x", strCurrentToken.length());
    
    strcat(errorMessage, (const char*)F("err 1; Unknown OP."));
    strcat(errorMessage, (const char*)F(" Len: ["));
    strcat(errorMessage, outputBuf);
    strcat(errorMessage, (const char*)F("]. Chars: ["));

    for (byte i = 0; i < strCurrentToken.length() - 1  && i < 0xFFFF; i++) {
      sprintf(outputBuf, "0x%02x", (byte)currentToken[i]);
      strcat(errorMessage, outputBuf);
      strcat(errorMessage, (const char*)F(", "));
    }
    sprintf(outputBuf, "0x%02x", (byte)currentToken[strCurrentToken.length() - 1]);
    strcat(errorMessage, outputBuf);
    strcat(errorMessage, (const char*)F("]: "));
    strcat(errorMessage, currentToken);
    return false;
  }

  return true;
}

void ICACHE_FLASH_ATTR execInputCommands(byte command, unsigned int *params, byte paramsLength, char resMessage[], char errorMessage[], char echoMessage[], int resMessageLength, int errorMessageLength, int echoMessageLength) {
  char outputBuf[32];

  if (resMessage != NULL) {
    memset(resMessage, '\0', sizeof(char) * resMessageLength);
  }

  if (errorMessage != NULL) {
    memset(errorMessage, '\0', sizeof(char) * errorMessageLength);
  }

  if (echoMessage != NULL) {
    memset(echoMessage, '\0', sizeof(char) * echoMessageLength);
  }

  switch(command) {
    case OP_NOP: {
      if (echoMessage != NULL) {
        strcat_P(echoMessage, (const char*)F("nop"));
      }
      
      if (resMessage != NULL) {
        strcat_P(resMessage, (const char*)F("nop"));
      }
    }
    break;
    case OP_JMP: {
      if (echoMessage != NULL) {
        strcat_P(echoMessage, (const char*)F("jmp"));
        sprintf(outputBuf, " 0x%04x", params[0]);
        strcat(echoMessage, (const char*)outputBuf);
      }

      if (paramsLength < 1) {
        if (errorMessage != NULL) {
          strcat_P(errorMessage, (const char*)F("err 2; jmp ("STR_OP_JMP") takes 1 param"));
        }
        break;
      }
      setNewAddress(params[0]);
      if (resMessage != NULL) {
        sprintf(outputBuf, "0x%04x", params[0]);
        strcat(resMessage, (const char*)outputBuf);
      }
    }
    break;
    case OP_AIC: {
      if (echoMessage != NULL) {
        strcat_P(echoMessage, (const char*)F("aic"));
      }
      setNewAddress(currentAddress + 1);
      if (resMessage != NULL) {
        sprintf(outputBuf, "0x%04x", currentAddress);
        strcat(resMessage, (const char*)outputBuf);
      }
    }
    break;
    case OP_ADC: {
      if (echoMessage != NULL) {
        strcat_P(echoMessage, (const char*)F("adc"));
      }
      setNewAddress(currentAddress - 1);
      if (resMessage != NULL) {
        sprintf(outputBuf, "0x%04x", currentAddress);
        strcat(resMessage, (const char*)outputBuf);
      }
    }
    break;
    case OP_GET: {
      if (paramsLength == 0) {
        if (echoMessage != NULL) {
          strcat_P(echoMessage, (const char*)F("get"));
        }
        if (resMessage != NULL) {
          sprintf(outputBuf, "0x%04x", currentAddress);
          strcat(resMessage, (const char*)outputBuf);
          sprintf(outputBuf, " 0x%02x", memRead);
          strcat(resMessage, (const char*)outputBuf);
        }
      }
    }
    break;
    case OP_GETC: {
      if (paramsLength == 1) {
        if (echoMessage != NULL) {
          strcat_P(echoMessage, (const char*)F("get "));
          sprintf(outputBuf, "0x%04x", params[0]);
          strcat(echoMessage, (const char*)outputBuf);
        }
        const byte readRes = exEepromReadByte(&memRead, primaryEepromPointer, params[0], EEPROM_READ_FAILURE);
        if (resMessage != NULL) {
          sprintf(outputBuf, "0x%04x", params[0]);
          strcat(resMessage, (const char*)outputBuf);
          sprintf(outputBuf, " 0x%02x", memRead);
          strcat(resMessage, (const char*)outputBuf);
        }
      } else {
        if (errorMessage != NULL) {
          strcat_P(errorMessage, (const char*)F("err 3; get ("STR_OP_GETC") takes 1 param"));
        }
        break;
      }
    }
    break;
    case OP_MOV: {
      if (echoMessage != NULL) {
        strcat_P(echoMessage, (const char*)F("mov "));
        sprintf(outputBuf, "0x%04x", params[0]);
        strcat(echoMessage, (const char*)outputBuf);
        sprintf(outputBuf, " 0x%02x", params[1] & 0xFF);
        strcat(echoMessage, (const char*)outputBuf);
      }
      if (paramsLength == 2) {
        setNewAddress(params[0]);
        exEepromWriteByte(primaryEepromPointer, currentAddress, params[1] & 0xFF);
        const byte readRes = exEepromReadByte(&memRead, primaryEepromPointer, currentAddress, EEPROM_READ_FAILURE);
        if (resMessage != NULL) {
          sprintf(outputBuf, "0x%04x", currentAddress);
          strcat(resMessage, (const char*)outputBuf);
          sprintf(outputBuf, " 0x%02x", memRead);
          strcat(resMessage, (const char*)outputBuf);
        }
      } else {
        if (errorMessage != NULL) {
          strcat_P(errorMessage, (const char*)F("err 4; mov ("STR_OP_MOV") takes 2 params"));
        }
        break;
      }
    }
    break;
    case OP_L2M: {
      if (echoMessage != NULL) {
        strcat_P(echoMessage, (const char*)F("l2m "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        strcat(echoMessage, (const char*)outputBuf);
      }
      if (paramsLength == 1) {
        exEepromWriteByte(primaryEepromPointer, currentAddress, params[0] & 0xFF);
        const byte readRes = exEepromReadByte(&memRead, primaryEepromPointer, currentAddress, EEPROM_READ_FAILURE);
        if (resMessage != NULL) {
          sprintf(outputBuf, "0x%04x", currentAddress);
          strcat(resMessage, (const char*)outputBuf);
          sprintf(outputBuf, " 0x%02x", memRead);
          strcat(resMessage, (const char*)outputBuf);
        }
      } else {
        if (errorMessage != NULL) {
          strcat_P(errorMessage, (const char*)F("err 5; "STR_OP_L2M" takes 1 params"));
        }
        break;
      }
    }
    break;
    case OP_PI2CC: {
      if (echoMessage != NULL) {
        strcat_P(echoMessage, (const char*)F("pi2c "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        strcat(echoMessage, (const char*)outputBuf);
      }
      if (paramsLength == 1) {
        primaryEepromPointer = params[0];
        if (resMessage != NULL) {
          sprintf(outputBuf, "0x%02x", params[0]);
          strcat(resMessage, (const char*)outputBuf);
        }
        delay(10);
      } else {
        if (errorMessage != NULL) {
          strcat_P(errorMessage, (const char*)F("err 6; "STR_OP_PI2C" takes 1 params"));
        }
        break;
      }
    }
    break;
    case OP_SI2CC: {
      if (echoMessage != NULL) {
        strcat_P(echoMessage, (const char*)F("si2c "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        strcat(echoMessage, (const char*)outputBuf);
      }
      if (paramsLength == 1) {
        secondaryEepromAddress = params[0];
        if (resMessage != NULL) {
          sprintf(outputBuf, "0x%02x", params[0]);
          strcat(resMessage, (const char*)outputBuf);
        }
        delay(10);
      } else {
        if (errorMessage != NULL) {
          strcat_P(errorMessage, (const char*)F("err 7; "STR_OP_SI2C" takes 1 params"));
        }
        break;
      }
    }
    break;
    case OP_PI2C: {
      if (paramsLength == 0) {
        if (echoMessage != NULL) {
          strcat_P(echoMessage, (const char*)F("pi2c"));
        }
        if (resMessage != NULL) {
          sprintf(outputBuf, "0x%02x", primaryEepromPointer);
          strcat(resMessage, (const char*)outputBuf);
        }
      }
    }
    break;
    case OP_SI2C: {
      if (paramsLength == 0) {
        if (echoMessage != NULL) {
          strcat_P(echoMessage, (const char*)F("si2c"));
        }
        if (resMessage != NULL) {
          sprintf(outputBuf, "0x%02x", secondaryEepromAddress);
          strcat(resMessage, (const char*)outputBuf);
        }
      }
    }
    break;
    case OP_PERC: {
      if (paramsLength == 0) {
        if (echoMessage != NULL) {
          Serial.println(F("per"));
        }
        if (resMessage != NULL) {
          sprintf(outputBuf, "0x%02x", primaryEepromPointer);
          strcat(resMessage, (const char*)outputBuf);
        }
      }
    }
    break;
    case OP_PER: {
      if (echoMessage != NULL) {
        strcat_P(echoMessage, (const char*)F("per "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        strcat(echoMessage, (const char*)outputBuf);
      }
      if (paramsLength == 1) {
        if (params[0] == 0x00) {
          primaryEepromPointer = EEPROM_INTERNAL_ADDR;
        } else if (params[0] == 0x01) {
          primaryEepromPointer = primaryEepromAddress;
        } else if (params[0] == 0x02) {
          primaryEepromPointer = secondaryEepromAddress;
        } else {
          primaryEepromPointer = primaryEepromAddress;
        }
        if (resMessage != NULL) {
          sprintf(outputBuf, "0x%02x", params[0]);
          strcat(resMessage, (const char*)outputBuf);
          strcat_P(resMessage, (const char*)F("; "));
          sprintf(outputBuf, "0x%02x", primaryEepromPointer);
          strcat(resMessage, (const char*)outputBuf);
        }
        delay(10);
      } else {
        if (errorMessage != NULL) {
          strcat_P(errorMessage, (const char*)F("err 8; "STR_OP_PER" takes 1 params"));
        }
        break;
      }
    }
    break;
    case OP_SERC: {
      if (paramsLength == 0) {
        if (echoMessage != NULL) {
          strcat_P(echoMessage, (const char*)F("ser "));
        }
        if (resMessage != NULL) {
          sprintf(outputBuf, "0x%02x", secondaryEepromPointer);
          strcat(resMessage, (const char*)outputBuf);
        }
      }
    }
    break;
    case OP_SER: {
      if (echoMessage != NULL) {
        strcat_P(echoMessage, (const char*)F("ser "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        strcat(echoMessage, (const char*)outputBuf);
      }
      if (paramsLength == 1) {
        if (params[0] == 0x00) {
          secondaryEepromPointer = EEPROM_INTERNAL_ADDR;
        } else if (params[0] == 0x01) {
          secondaryEepromPointer = primaryEepromAddress;
        } else if (params[0] == 0x02) {
          secondaryEepromPointer = secondaryEepromAddress;
        } else {
          secondaryEepromPointer = secondaryEepromAddress;
        }
        if (resMessage != NULL) {
          sprintf(outputBuf, "0x%02x", params[0]);
          strcat(resMessage, (const char*)outputBuf);
          strcat(resMessage, (const char*)F("; "));
          sprintf(outputBuf, "0x%02x", secondaryEepromPointer);
          strcat(resMessage, (const char*)outputBuf);
        }
        delay(10);
      } else {
        if (errorMessage != NULL) {
          strcat_P(errorMessage, (const char*)F("err 9; "STR_OP_SER" takes 1 params"));
        }
        break;
      }
    }
    break;
    case OP_PCPY: {
      if (echoMessage != NULL) {
        strcat_P(echoMessage, (const char*)F("pcpy "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        strcat(echoMessage, (const char*)outputBuf);
      }
      if (paramsLength == 2) {
        const unsigned int startAddress = currentAddress;
        const int maxCopySize = 4096;
        unsigned int memAddrOffset = 0;
        byte tmpMem = 0;
        
        for (memAddrOffset = 0; memAddrOffset < maxCopySize && memAddrOffset < params[1]; memAddrOffset++) {
          setNewAddress(startAddress + memAddrOffset);
          readAndDisplayMemory();
          readAndDisplayAddress();
          const byte readRes = exEepromReadByte(&tmpMem, primaryEepromPointer, currentAddress, EEPROM_READ_FAILURE);
          exEepromWriteByte(secondaryEepromPointer, currentAddress, params[0] & 0xFF);
          ioMod.setDisplay(currentScreenValue);
          if (resMessage != NULL) {
            sprintf(outputBuf, "0x%02x ", tmpMem);
            strcat(resMessage, (const char*)outputBuf);
          }
        }
        
        if (resMessage != NULL) {
          strcat_P(resMessage, (const char*)F("\n"));
          sprintf(outputBuf, "0x%02x", memAddrOffset);
          strcat(resMessage, (const char*)outputBuf);
        }
        delay(10);
      } else {
        if (errorMessage != NULL) {
          strcat_P(errorMessage, (const char*)F("err 10; "STR_OP_INT" takes 2 params"));
        }
        break;
      }
    }
    break;
    case OP_INT: {
      if (echoMessage != NULL) {
        strcat_P(echoMessage, (const char*)F("int "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        strcat(echoMessage, (const char*)outputBuf);
      }
      if (paramsLength == 1) {
        assemblyInterpretMode = params[0] == 0 ? false : true;
        if (resMessage != NULL) {
          sprintf(outputBuf, "0x%02x", params[0]);
          strcat(resMessage, (const char*)outputBuf);
          strcat_P(resMessage, (const char*)F("; "));
          strcat(resMessage, assemblyInterpretMode ? (const char*)F("true") : (const char*)F("false"));
        }
        delay(10);
      } else {
        if (errorMessage != NULL) {
          strcat_P(errorMessage, (const char*)F("err 11; "STR_OP_INT" takes 1 params"));
        }
        break;
      }
    }
    break;
    case OP_ECH: {
      if (echoMessage != NULL) {
        strcat_P(echoMessage, (const char*)F("ech "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        strcat(echoMessage, (const char*)outputBuf);
      }
      if (paramsLength == 1) {
        serialEchoCommand = params[0] == 0 ? false : true;
        if (resMessage != NULL) {
          sprintf(outputBuf, "0x%02x", params[0]);
          strcat(resMessage, (const char*)outputBuf);
          strcat_P(resMessage, (const char*)F("; "));
          strcat_P(resMessage, serialEchoCommand ? (const char*)F("true") : (const char*)F("false"));
        }
        delay(5);
      } else {
        if (errorMessage != NULL) {
          strcat_P(errorMessage, (const char*)F("err 12; "STR_OP_ECH" takes 1 params"));
        }
        break;
      }
    }
    break;
    case OP_ADMP: {
      if (echoMessage != NULL) {
        strcat_P(echoMessage, (const char*)F("admp "));
        sprintf(outputBuf, "0x%04x", params[0]);
        strcat(echoMessage, (const char*)outputBuf);
        sprintf(outputBuf, " 0x%02x", params[1] & 0xFF);
        strcat(echoMessage, (const char*)outputBuf);
      }
      if (paramsLength == 2) {
        setNewAddress(params[0]);
        const int maxCopySize = 4096;
        unsigned int memAddrOffset = 0;
        byte tmpMemVal = 0;
        
        for (memAddrOffset = 0; memAddrOffset < maxCopySize && memAddrOffset < params[1]; memAddrOffset++) {
          setNewAddress(params[0] + memAddrOffset);
          readAndDisplayMemory();
          readAndDisplayAddress();
          const byte readRes = exEepromReadByte(&tmpMemVal, primaryEepromPointer, currentAddress, EEPROM_READ_FAILURE);
          ioMod.setDisplay(currentScreenValue);
          if (resMessage != NULL) {
            sprintf(outputBuf, "0x%02x ", tmpMemVal);
            strcat(resMessage, (const char*)outputBuf);
          }
        }
        if (resMessage != NULL) {
          strcat_P(resMessage, (const char*)F("\n"));
          sprintf(outputBuf, "0x%02x", memAddrOffset);
          strcat(resMessage, (const char*)outputBuf);
        }
        delay(10);
      } else {
        if (errorMessage != NULL) {
          strcat_P(errorMessage, (const char*)F("err 13; "STR_OP_ADMP" takes 2 params"));
        }
        break;
      }
    }
    break;
    case OP_CDMP: {
      if (echoMessage != NULL) {
        strcat_P(echoMessage, (const char*)F("cdmp "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        strcat(echoMessage, (const char*)outputBuf);
      }
      if (paramsLength == 1) {
        const unsigned int startAddress = currentAddress;
        const int maxCopySize = 4096;
        unsigned int memAddrOffset = 0;
        byte tmpMemVal = 0;
        
        for (memAddrOffset = 0; memAddrOffset < maxCopySize && memAddrOffset < params[0]; memAddrOffset++) {
          setNewAddress(startAddress + memAddrOffset);
          readAndDisplayMemory();
          readAndDisplayAddress();
          const byte readRes = exEepromReadByte(&tmpMemVal, primaryEepromPointer, currentAddress, EEPROM_READ_FAILURE);
          ioMod.setDisplay(currentScreenValue);
          if (resMessage != NULL) {
            sprintf(outputBuf, "0x%02x ", tmpMemVal);
            strcat(resMessage, (const char*)outputBuf);
          }
        }
        if (resMessage != NULL) {
          strcat(resMessage, (const char*)F("\n"));
          sprintf(outputBuf, "0x%02x", memAddrOffset);
          strcat(resMessage, (const char*)outputBuf);
        }
        delay(10);
      } else {
        if (errorMessage != NULL) {
          strcat_P(errorMessage, (const char*)F("err 14; "STR_OP_CDMP" takes 1 params"));
        }
        break;
      }
    }
    break;
    case OP_RETD: {
      if (echoMessage != NULL) {
        strcat_P(echoMessage, (const char*)F("ret"));
      }
      if (resMessage != NULL) {
        strcat_P(resMessage, (const char*)F("ret "));
        sprintf(outputBuf, "0x%02x", DEFAULT_RUN_MODE);
        strcat(resMessage, (const char*)outputBuf);
      }
      delay(10);
      currentMode = DEFAULT_RUN_MODE;
      ioMod.setLED(TM1638_COLOR_RED, 0);
      ioMod.setLED(TM1638_COLOR_RED, 1);
      ioMod.setLED(TM1638_COLOR_RED, 2);
      ioMod.setLED(TM1638_COLOR_RED, 3);
    }
    break;
    case OP_RET: {
      if (paramsLength == 1) {
        if (echoMessage != NULL) {
          strcat_P(echoMessage, (const char*)F("ret "));
          sprintf(outputBuf, "0x%02x", params[0]);
          strcat(echoMessage, (const char*)outputBuf);
        }
        if (resMessage != NULL) {
          strcat_P(resMessage, (const char*)F("ret "));
          sprintf(outputBuf, "0x%02x", params[0]);
          strcat(resMessage, (const char*)outputBuf);
        }
        delay(10);
        currentMode = params[0];
      } else {
        if (errorMessage != NULL) {
          strcat_P(errorMessage, (const char*)F("err 15; ret ("STR_OP_RET") takes 1 params"));
        }
        break;
      }
    }
    break;
    case OP_I2C: {
      if (echoMessage != NULL) {
        strcat_P(echoMessage, (const char*)F("i2c"));
      }
      const byte totalDeviceCount = 127;
      int goodDevices[totalDeviceCount];
      int badDevices[totalDeviceCount];
      byte goodFoundDevicesLength = 0;
      byte badFoundDevicesLength = 0;

      scanI2CDevices(goodDevices, (int)totalDeviceCount, &goodFoundDevicesLength, badDevices, (int)totalDeviceCount, &badFoundDevicesLength);
      if (resMessage != NULL) {
        if (goodFoundDevicesLength > 0) {
          strcat_P(resMessage, (const char*)F("Good: "));
          for (int i = 0; i < goodFoundDevicesLength ; i++) {
            sprintf(outputBuf, "0x%02x ", goodDevices[i]);
            strcat(resMessage, (const char*)outputBuf);
          }
          strcat_P(resMessage, (const char*)F("\n"));
        }
  
        if (badFoundDevicesLength > 0) {
          strcat_P(resMessage, (const char*)F("Bad: "));
          for (int i = 0; i < badFoundDevicesLength ; i++) {
            sprintf(outputBuf, "0x%02 ", badDevices[i]);
            strcat(resMessage, (const char*)outputBuf);
          }
          strcat_P(resMessage, (const char*)F("\n"));
        }
      }
    }
    break;
    case OP_MEM: {
      if (echoMessage != NULL) {
        strcat_P(echoMessage, (const char*)F("mem"));
      }
      bool loadedMemory = readEepromSettings();
      if (resMessage != NULL) {
        sprintf(outputBuf, "0x%02x", loadedMemory);
        strcat(resMessage, (const char*)outputBuf);
        strcat_P(resMessage, (const char*)F("; "));
        strcat_P(resMessage, loadedMemory ? (const char*)F("true") : (const char*)F("false"));
      }
    }
    break;
    case OP_DNUM: {
      if (paramsLength == 1) {
        useNumeric = params[0];
        if (echoMessage != NULL) {
          strcat_P(echoMessage, (const char*)F("dnum "));
          sprintf(outputBuf, "0x%02x", params[0]);
          strcat(echoMessage, (const char*)outputBuf);
        }
        if (resMessage != NULL) {
          sprintf(outputBuf, "0x%02x", params[0]);
          strcat(resMessage, (const char*)outputBuf);
          strcat_P(resMessage, (const char*)F("; "));
          strcat_P(resMessage, useNumeric != 0 ? (const char*)F("true") : (const char*)F("false"));
        }
        delay(10);
      } else {
        if (errorMessage != NULL) {
          strcat_P(errorMessage, (const char*)F("err 15; dnum ("STR_OP_DNUM") takes 1 param"));
        }
        break;
      }
    }
    break;
    case OP_RST: {
      if (echoMessage != NULL) {
        strcat_P(echoMessage, (const char*)F("rst"));
      }
      if (resMessage != NULL) {
        strcat_P(resMessage, (const char*)F("RESET"));
      }
      delay(10);
      Reset();
    }
    break;
    case OP_STAT: {
      if (paramsLength == 0) {
        if (echoMessage != NULL) {
          strcat_P(echoMessage, (const char*)F("stat"));
        }
        if (resMessage != NULL) {
          sprintf(outputBuf, "Uptime: %d\r\n", millis());
          strcat(resMessage, (const char*)outputBuf);
          sprintf(outputBuf, "Heap: %d\r\n", heapAvailable());
          strcat(resMessage, (const char*)outputBuf);
          sprintf(outputBuf, "Stack: %d\r\n", stackAvailable());
          strcat(resMessage, (const char*)outputBuf);
          sprintf(outputBuf, "IP: %s\r\n", WiFi.localIP().toString().c_str());
          strcat(resMessage, (const char*)outputBuf);
          sprintf(outputBuf, "RSSI: %d\r\n", (long)WiFi.RSSI());
          strcat(resMessage, (const char*)outputBuf);
        }
      }
    }
    break;
  }
  
  readAndDisplayAddress();
  readAndDisplayMemory();
  ioMod.setDisplay(currentScreenValue);
}
