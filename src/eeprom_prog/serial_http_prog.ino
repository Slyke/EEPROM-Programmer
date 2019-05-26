
void ICACHE_FLASH_ATTR processCommandInput(char* commSeg) {
  processCommandInput(commSeg, assemblyInterpretMode);
}

void ICACHE_FLASH_ATTR serialCommandInput() {
  if (Serial.available()) {
    String serialResponse = Serial.readStringUntil('\n');
    serialResponse.trim();
    char *commSeg = (char *)serialResponse.c_str();
    processCommandInput(commSeg); 
  }
}

void ICACHE_FLASH_ATTR processCommandInput(char* commSeg, bool assemblyMode) {
  char *remaining;
  char *currentToken;
  
  byte paramsLength = 0;
  byte commandDecodeRet[2] = {0};
  unsigned int params[2] = {0};

  if (assemblyMode) {
    while ((remaining = strtok_r(commSeg, ";", &commSeg)) != NULL) {
      commandDecodeRet[0] = 0;
      commandDecodeRet[1] = 0;
      paramsLength = 0;
      params[0] = 0;
      params[1] = 0;
      char *commandOp = strtok_r(remaining, " ", &remaining);
      commandDecode(commandDecodeRet, commandOp, remaining != NULL ? strlen(remaining) : 0, NULL);

      while ((currentToken = strtok_r(remaining, " ", &remaining)) != NULL && paramsLength < OP_PROCESS_ARR_LIMIT && paramsLength < commandDecodeRet[1]) {
        int res;
        stringToNumber((int *)&res, currentToken);
        params[paramsLength] = res;
        paramsLength++;
      }

      char byteArray[paramsLength + 1];
      byteArray[0] = commandDecodeRet[0];

      for (int i = 0; i < MAX_OP_PARAM_LENGTH && i < commandDecodeRet[1]; i++) {
        byteArray[i + 1] = params[i];
      }
      
      processCommandInputByteCode(byteArray, commandDecodeRet[1] + 1);
    }
  } else {
    remaining = commSeg;
    while ((currentToken = strtok_r(remaining, " ", &remaining)) != NULL) {
      // processCommandInputByteCode
    }
  }
}

void ICACHE_FLASH_ATTR processCommandInputByteCode(char* byteCodeArr, int byteCodeLength) {
  char *remaining;
  
  byte paramsLength = 0;
  byte commandDecodeRet[2] = {0};
  unsigned int params[2] = {0};

  for (int i = 0; i < byteCodeLength && i < OP_PROCESS_ARR_LIMIT; i++) {
    if (commandDecodeRet[0] > 0) {
      // Command parameter parser
      commandParamParse(byteCodeArr, i, params, paramsLength);

      if (paramsLength >= commandDecodeRet[1]) {
        execInputCommands(commandDecodeRet[0], params, paramsLength);
        i += paramsLength;
        commandDecodeRet[0] = 0;
        commandDecodeRet[1] = 0;
        paramsLength = 0;
        params[0] = 0;
        params[1] = 0;
      }
    } else {
      // Command decoder
      commandDecodeRet[0] = 0;
      paramsLength = 0;
      params[0] = 0;
      params[1] = 0;
      
      commandToOpAndParam(byteCodeArr[i], commandDecodeRet);
      paramsLength = commandDecodeRet[1];
      if (commandDecodeRet[1] == 0) {
        execInputCommands(commandDecodeRet[0], params, paramsLength);
        paramsLength = 0;
 
      }
    }
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
    case OP_JMP: case OP_GET: case OP_L2M: case OP_PI2CC: case OP_SI2CC: case OP_PERC: case OP_SERC: case OP_INT: case OP_ECH: case OP_CDMP: case OP_RET:
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
  } else if (strcmp(currentToken, "nop") == 0) {
    commandToOpAndParam(OP_NOP, ret);
  } else if (strcmp(currentToken, "jmp") == 0) {
    commandToOpAndParam(OP_JMP, ret);
  } else if (strcmp(currentToken, "aic") == 0 || strcmp(currentToken, "inc") == 0) {
    commandToOpAndParam(OP_AIC, ret);
  } else if (strcmp(currentToken, "adc") == 0 || strcmp(currentToken, "dec") == 0) {
    commandToOpAndParam(OP_ADC, ret);
  } else if (strcmp(currentToken, "get") == 0) {
    commandToOpAndParam(OP_GET, ret);
    if (remainingLength == 0) {
      commandToOpAndParam(OP_GETC, ret);
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
  } else if (strcmp(currentToken, "ret") == 0) {
    commandToOpAndParam(OP_RET, ret);
    if (remainingLength == 0) {
      commandToOpAndParam(OP_RETD, ret);
    }
  } else if (strcmp(currentToken, "rst") == 0) {
    commandToOpAndParam(OP_RST, ret);
  } else {
    char outputBuf[32];
    String strCurrentToken(currentToken);
    Serial.print(F("err 1; Unknown OP."));
    Serial.print(F(" Len: ["));
    sprintf(outputBuf, "0x%02x", strCurrentToken.length());
    Serial.print(outputBuf);
    Serial.print(F("]. Chars: ["));
    for (byte i = 0; i < strCurrentToken.length() - 1  && i < 0xFFFF; i++) {
      sprintf(outputBuf, "0x%02x", (byte)currentToken[i]);
      Serial.print(outputBuf);
      Serial.print(", ");
    }
    sprintf(outputBuf, "0x%02x", (byte)currentToken[strCurrentToken.length() - 1]);
    Serial.print(outputBuf);
    Serial.print("]: ");
    Serial.println(currentToken);
    return false;
  }

  return true;
}

void ICACHE_FLASH_ATTR execInputCommands(byte command, unsigned int *params, byte paramsLength) {
  char outputBuf[32];
  switch(command) {
    case OP_NOP: {
      Serial.println(F("nop"));
    }
    break;
    case OP_JMP: {
      if (serialEchoCommand) {
        Serial.print(F("jmp"));
        sprintf(outputBuf, " 0x%04x", params[0]);
        Serial.println(outputBuf);
      }
      if (paramsLength < 1) {
        Serial.println(F("err 2; jmp ("STR_OP_JMP") takes 1 param"));
        break;
      }
      setNewAddress(params[0]);
      sprintf(outputBuf, "0x%04x", params[0]);
      Serial.println(outputBuf);
    }
    break;
    case OP_AIC: {
      if (serialEchoCommand) {
        Serial.println(F("aic"));
      }
      setNewAddress(currentAddress + 1);
      sprintf(outputBuf, "0x%04x", currentAddress);
      Serial.println(outputBuf);
    }
    break;
    case OP_ADC: {
      if (serialEchoCommand) {
        Serial.println(F("adc"));
      }
      setNewAddress(currentAddress - 1);
      sprintf(outputBuf, "0x%04x", currentAddress);
      Serial.println(outputBuf);
    }
    break;
    case OP_GETC: {
      if (paramsLength == 0) {
        if (serialEchoCommand) {
          Serial.println(F("get"));
        }
        sprintf(outputBuf, "0x%04x", currentAddress);
        Serial.print(outputBuf);
        sprintf(outputBuf, " 0x%02x", memRead);
        Serial.println(outputBuf);
      }
    }
    break;
    case OP_GET: {
      if (paramsLength == 1) {
        if (serialEchoCommand) {
          Serial.print(F("get "));
          sprintf(outputBuf, "0x%04x", params[0]);
          Serial.println(outputBuf);
        }
        const byte readRes = exEepromReadByte(&memRead, primaryEepromReader, params[0], EEPROM_READ_FAILURE);
        sprintf(outputBuf, "0x%04x", params[0]);
        Serial.print(outputBuf);
        sprintf(outputBuf, " 0x%02x", memRead);
        Serial.println(outputBuf);
      } else {
        Serial.println(F("err 3; get ("STR_OP_GET") takes 1 param"));
        break;
      }
    }
    break;
    case OP_MOV: {
      if (serialEchoCommand) {
        Serial.print(F("mov "));
        sprintf(outputBuf, "0x%04x", params[0]);
        Serial.print(outputBuf);
        sprintf(outputBuf, " 0x%02x", params[1] & 0xFF);
        Serial.println(outputBuf);
      }
      if (paramsLength == 2) {
        setNewAddress(params[0]);
        exEepromWriteByte(primaryEepromReader, currentAddress, params[1] & 0xFF);
        const byte readRes = exEepromReadByte(&memRead, primaryEepromReader, currentAddress, EEPROM_READ_FAILURE);
        sprintf(outputBuf, "0x%04x", currentAddress);
        Serial.print(outputBuf);
        sprintf(outputBuf, " 0x%02x", memRead);
        Serial.println(outputBuf);
      } else {
        Serial.println(F("err 4; mov ("STR_OP_MOV") takes 2 params"));
        break;
      }
    }
    break;
    case OP_L2M: {
      if (serialEchoCommand) {
        Serial.print(F("l2m "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        Serial.println(outputBuf);
      }
      if (paramsLength == 1) {
        exEepromWriteByte(primaryEepromReader, currentAddress, params[0] & 0xFF);
        const byte readRes = exEepromReadByte(&memRead, primaryEepromReader, currentAddress, EEPROM_READ_FAILURE);
        sprintf(outputBuf, "0x%04x", currentAddress);
        Serial.print(outputBuf);
        sprintf(outputBuf, " 0x%02x", memRead);
        Serial.println(outputBuf);
      } else {
        Serial.println(F("err 8; "STR_OP_L2M" takes 1 params"));
        break;
      }
    }
    break;
    case OP_PI2CC: {
      if (paramsLength == 0) {
        if (serialEchoCommand) {
          Serial.println(F("pi2c"));
        }
        sprintf(outputBuf, "0x%02x", secondaryEepromAddress);
        Serial.println(outputBuf);
      }
    }
    break;
    case OP_SI2CC: {
      if (paramsLength == 0) {
        if (serialEchoCommand) {
          Serial.println(F("si2c"));
        }
        sprintf(outputBuf, "0x%02x", secondaryEepromAddress);
        Serial.println(outputBuf);
      }
    }
    break;
    case OP_PI2C: {
      if (serialEchoCommand) {
        Serial.print(F("pi2c "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        Serial.println(outputBuf);
      }
      if (paramsLength == 1) {
        primaryEepromAddress = params[0];
        sprintf(outputBuf, "0x%02x", params[0]);
        Serial.println(outputBuf);
        delay(10);
      } else {
        Serial.println(F("err 16; "STR_OP_PI2C" takes 1 params"));
        break;
      }
    }
    break;
    case OP_SI2C: {
      if (serialEchoCommand) {
        Serial.print(F("si2c "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        Serial.println(outputBuf);
      }
      if (paramsLength == 1) {
        secondaryEepromAddress = params[0];
        sprintf(outputBuf, "0x%02x", params[0]);
        Serial.println(outputBuf);
        delay(10);
      } else {
        Serial.println(F("err 16; "STR_OP_SI2C" takes 1 params"));
        break;
      }
    }
    break;
    case OP_PERC: {
      if (paramsLength == 0) {
        if (serialEchoCommand) {
          Serial.println(F("per"));
        }
        sprintf(outputBuf, "0x%02x", primaryEepromReader);
        Serial.println(outputBuf);
      }
    }
    break;
    case OP_PER: {
      if (serialEchoCommand) {
        Serial.print(F("per "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        Serial.println(outputBuf);
      }
      if (paramsLength == 1) {
        if (params[0] == 0x00) {
          primaryEepromReader = EEPROM_INTERNAL_ADDR;
        } else if (params[0] == 0x01) {
          primaryEepromReader = primaryEepromAddress;
        } else if (params[0] == 0x02) {
          primaryEepromReader = secondaryEepromAddress;
        } else {
          primaryEepromReader = primaryEepromAddress;
        }
        sprintf(outputBuf, "0x%02x", params[0]);
        Serial.print(outputBuf);
        Serial.print(F("; "));
        sprintf(outputBuf, "0x%02x", primaryEepromReader);
        Serial.println(outputBuf);
        delay(10);
      } else {
        Serial.println(F("err 16; "STR_OP_PER" takes 1 params"));
        break;
      }
    }
    break;
    case OP_SERC: {
      if (paramsLength == 0) {
        if (serialEchoCommand) {
          Serial.println(F("ser"));
        }
        sprintf(outputBuf, "0x%02x", secondaryEepromReader);
        Serial.println(outputBuf);
      }
    }
    break;
    case OP_SER: {
      if (serialEchoCommand) {
        Serial.print(F("ser "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        Serial.println(outputBuf);
      }
      if (paramsLength == 1) {
        if (params[0] == 0x00) {
          secondaryEepromReader = EEPROM_INTERNAL_ADDR;
        } else if (params[0] == 0x01) {
          secondaryEepromReader = primaryEepromAddress;
        } else if (params[0] == 0x02) {
          secondaryEepromReader = secondaryEepromAddress;
        } else {
          secondaryEepromReader = secondaryEepromAddress;
        }
        sprintf(outputBuf, "0x%02x", params[0]);
        Serial.print(outputBuf);
        Serial.print(F("; "));
        sprintf(outputBuf, "0x%02x", secondaryEepromReader);
        Serial.println(outputBuf);
        delay(10);
      } else {
        Serial.println(F("err 16; "STR_OP_SER" takes 1 params"));
        break;
      }
    }
    break;
    case OP_PCPY: {
      if (serialEchoCommand) {
        Serial.print(F("pcpy "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        Serial.println(outputBuf);
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
          const byte readRes = exEepromReadByte(&tmpMem, primaryEepromReader, currentAddress, EEPROM_READ_FAILURE);
          exEepromWriteByte(secondaryEepromReader, currentAddress, params[0] & 0xFF);
          ioMod.setDisplay(currentScreenValue);
          sprintf(outputBuf, "0x%02x ", tmpMem);
          Serial.print(outputBuf);
        }
        
        Serial.print('\n');
        sprintf(outputBuf, "0x%02x", memAddrOffset);
        Serial.println(outputBuf);
        delay(10);
      } else {
        Serial.println(F("err 16; "STR_OP_INT" takes 2 params"));
        break;
      }
    }
    break;
    case OP_INT: {
      if (serialEchoCommand) {
        Serial.print(F("int "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        Serial.println(outputBuf);
      }
      if (paramsLength == 1) {
        assemblyInterpretMode = params[0] == 0 ? false : true;
        sprintf(outputBuf, "0x%02x", params[0]);
        Serial.print(outputBuf);
        Serial.print("; ");
        Serial.println(assemblyInterpretMode ? F("true") : F("false"));
        delay(10);
      } else {
        Serial.println(F("err 16; "STR_OP_INT" takes 1 params"));
        break;
      }
    }
    break;
    case OP_ECH: {
      if (serialEchoCommand) {
        Serial.print(F("ech "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        Serial.println(outputBuf);
      }
      if (paramsLength == 1) {
        serialEchoCommand = params[0] == 0 ? false : true;
        sprintf(outputBuf, "0x%02x", params[0]);
        Serial.print(outputBuf);
        Serial.print(F("; "));
        Serial.println(serialEchoCommand ? F("true") : F("false"));
        delay(10);
      } else {
        Serial.println(F("err 32; "STR_OP_ECH" takes 1 params"));
        break;
      }
    }
    break;
    case OP_ADMP: {
      if (serialEchoCommand) {
        Serial.print(F("admp "));
        sprintf(outputBuf, "0x%04x", params[0]);
        Serial.print(outputBuf);
        sprintf(outputBuf, " 0x%02x", params[1] & 0xFF);
        Serial.println(outputBuf);
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
          const byte readRes = exEepromReadByte(&tmpMemVal, primaryEepromReader, currentAddress, EEPROM_READ_FAILURE);
          ioMod.setDisplay(currentScreenValue);
          sprintf(outputBuf, "0x%02x ", tmpMemVal);
          Serial.print(outputBuf);
        }
        
        Serial.print('\n');
        sprintf(outputBuf, "0x%02x", memAddrOffset);
        Serial.println(outputBuf);
        delay(10);
      } else {
        Serial.println(F("err 32; "STR_OP_ADMP" takes 2 params"));
        break;
      }
    }
    break;
    case OP_CDMP: {
      if (serialEchoCommand) {
        Serial.print(F("cdmp "));
        sprintf(outputBuf, "0x%02x", params[0] & 0xFF);
        Serial.println(outputBuf);
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
          const byte readRes = exEepromReadByte(&tmpMemVal, primaryEepromReader, currentAddress, EEPROM_READ_FAILURE);
          ioMod.setDisplay(currentScreenValue);
          sprintf(outputBuf, "0x%02x ", tmpMemVal);
          Serial.print(outputBuf);
        }
        
        Serial.print('\n');
        sprintf(outputBuf, "0x%02x", memAddrOffset);
        Serial.println(outputBuf);
        delay(10);
      } else {
        Serial.println(F("err 32; "STR_OP_CDMP" takes 1 params"));
        break;
      }
    }
    break;
    case OP_RETD: {
      if (serialEchoCommand) {
        Serial.println(F("ret"));
      }
      Serial.print(F("ret "));
      sprintf(outputBuf, "0x%02x", DEFAULT_RUN_MODE);
      Serial.println(outputBuf);
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
        if (serialEchoCommand) {
          Serial.print(F("ret "));
          sprintf(outputBuf, "0x%02x", params[0]);
          Serial.println(outputBuf);
        }
        Serial.print(F("ret "));
        sprintf(outputBuf, "0x%02x", params[0]);
        Serial.println(outputBuf);
        delay(10);
        currentMode = params[0];
      } else {
        Serial.println(F("err 64; ret ("STR_OP_RET") takes 1 params"));
        break;
      }
    }
    break;
    case OP_I2C: {
      if (serialEchoCommand) {
        Serial.println(F("i2c"));
      }
      const byte totalDeviceCount = 127;
      int goodDevices[totalDeviceCount];
      int badDevices[totalDeviceCount];
      byte goodFoundDevicesLength = 0;
      byte badFoundDevicesLength = 0;

      scanI2CDevices(goodDevices, (int)totalDeviceCount, &goodFoundDevicesLength, badDevices, (int)totalDeviceCount, &badFoundDevicesLength);

      if (goodFoundDevicesLength > 0) {
        Serial.print(F("Good: "));
        for (int i = 0; i < goodFoundDevicesLength ; i++) {
          sprintf(outputBuf, "0x%02x ", goodDevices[i]);
          Serial.print(outputBuf);
        }
        Serial.print('\n');
      }

      if (badFoundDevicesLength > 0) {
        Serial.print(F("Bad: "));
        for (int i = 0; i < badFoundDevicesLength ; i++) {
          sprintf(outputBuf, "0x%02 ", badDevices[i]);
          Serial.print(outputBuf);
        }
        Serial.print('\n');
      }
    }
    break;
    case OP_RST: {
      if (serialEchoCommand) {
        Serial.println(F("rst"));
      }
      Serial.println(F("RESET"));
      delay(10);
      Reset();
    }
    break;
  }
  readAndDisplayAddress();
  readAndDisplayMemory();
  ioMod.setDisplay(currentScreenValue);
}
