
void ICACHE_FLASH_ATTR httpHandleRoot() {
  DynamicJsonDocument JSONbuffer(256);
  char JSONmessageBuffer[256];
  
  JsonObject JSONencoder = JSONbuffer.to<JsonObject>();
  JSONencoder["payload"];
  JSONencoder["payload"]["meta"];
  JSONencoder["payload"][F("device")];
  JSONencoder["payload"][F("data")];

  JSONencoder["payload"]["meta"]["uptime"] = millis();
  JSONencoder["payload"]["meta"]["heapAvailable"] = heapAvailable();
  JSONencoder["payload"]["meta"]["stackAvailable"] = stackAvailable();

  JSONencoder["payload"][F("device")][F("ip")] = (String)WiFi.localIP().toString();
  JSONencoder["payload"][F("device")][F("rssi")] = (long)WiFi.RSSI();

  serializeJson(JSONencoder, JSONmessageBuffer);
  
  webServer.send(200, "application/json", JSONmessageBuffer);
}

void ICACHE_FLASH_ATTR httpHandleExec() {
  if (webServer.arg("command") == "") {
    char* errorMsg = strdup((const char*)F("/exec requires command param"));
    httpSendIntError(errorMsg);
    if (errorMsg) {
      free(errorMsg);
    }
    return;
  }

  webServer.send(200, "application/json", "");

}

void ICACHE_FLASH_ATTR httpSendIntError(int locIndex, char* errorChars, char* errorMessage) {
  DynamicJsonDocument JSONErrorBuffer(256);
  char JSONErrorMessageBuffer[256];
  
  JsonObject JSONErrorEncoder = JSONErrorBuffer.to<JsonObject>();
  httpEncodeIntError(JSONErrorEncoder, locIndex, errorChars, errorMessage);
  
  serializeJson(JSONErrorEncoder, JSONErrorMessageBuffer);
  webServer.send(400, "application/json", JSONErrorMessageBuffer);
}

void ICACHE_FLASH_ATTR httpSendIntError(char* errorMessage) {
  DynamicJsonDocument JSONErrorBuffer(256);
  char JSONErrorMessageBuffer[256];
  
  JsonObject JSONErrorEncoder = JSONErrorBuffer.to<JsonObject>();

  httpEncodeIntError(JSONErrorEncoder, errorMessage);

  serializeJson(JSONErrorEncoder, JSONErrorMessageBuffer);
  webServer.send(400, "application/json", JSONErrorMessageBuffer);
}

void ICACHE_FLASH_ATTR httpEncodeIntError(const JsonObject &JSONencoder, int locIndex, char* errorChars, char* errorMessage) {
  JSONencoder["payload"];
  JSONencoder["payload"]["meta"];
  JSONencoder["payload"]["status"];

  JSONencoder["payload"]["meta"]["uptime"] = millis();
  JSONencoder["payload"]["meta"]["heapAvailable"] = heapAvailable();
  JSONencoder["payload"]["meta"]["stackAvailable"] = stackAvailable();

  JSONencoder["payload"]["status"]["code"] = "400";
  JSONencoder["payload"]["status"]["codeMessage"] = F("Bad Request");
  JSONencoder["payload"]["status"][F("errorDescription")] = errorMessage;
  JSONencoder["payload"]["status"][F("location")] = locIndex;
  JSONencoder["payload"]["status"][F("characters")] = errorChars;
}

void ICACHE_FLASH_ATTR httpEncodeIntError(const JsonObject JSONencoder, char* errorMessage) {
  JSONencoder["payload"];
  JSONencoder["payload"]["meta"];
  JSONencoder["payload"]["status"];

  JSONencoder["payload"]["meta"]["uptime"] = millis();
  JSONencoder["payload"]["meta"]["heapAvailable"] = heapAvailable();
  JSONencoder["payload"]["meta"]["stackAvailable"] = stackAvailable();

  JSONencoder["payload"]["status"]["code"] = F("400");
  JSONencoder["payload"]["status"]["codeMessage"] = F("Bad Request");
  JSONencoder["payload"]["status"][F("errorDescription")] = errorMessage;
}

void ICACHE_FLASH_ATTR httpHandleInt() {
  if (webServer.hasArg("plain") == false) {
    char* errorMsg = strdup((const char*)F("Invalid character in OP list"));
    httpSendIntError(errorMsg);
    if (errorMsg) {
      free(errorMsg);
    }
    return;
  }

  if (strlen(webServer.arg("plain").c_str()) > OP_PROCESS_ARR_LIMIT) {
    char* errorMsg = strdup((const char*)F("POST body too large. Keep smaller than 512 bytes."));
    httpSendIntError(errorMsg);
    if (errorMsg) {
      free(errorMsg);
    }
    return;
  }
  return;
//  DynamicJsonDocument JSONbuffer(512);
//  deserializeJson(JSONbuffer, webServer.arg("plain"));
//
//  JsonArray opList = JSONbuffer["ops"];
//  char codeList[OP_PROCESS_ARR_LIMIT];
//  int byteIndex = 0;
//
//  for(JsonVariant elem: opList) {
//    if (byteIndex > OP_PROCESS_ARR_LIMIT) {
//      JSONbuffer.clear();
//      char* errorMsg = strdup((const char*)F("Byte array length exceeds 512"));
//      httpSendIntError(errorMsg);
//      if (errorMsg) {
//        free(errorMsg);
//      }
//      return;
//    }
//    if (elem.is<char*>()) {
//      unsigned int res;
//      char* inputChar = strdup(elem.as<char*>());
//      boolean numRes = stringToNumber((int *)&res, inputChar);
//      if (inputChar) {
//        free(inputChar);
//      }
//      if (!numRes) {
//        char* errorMsg = strdup((const char*)F("Invalid character in OP list"));
//        httpSendIntError(byteIndex, inputChar, errorMsg);
//        if (errorMsg) {
//          free(errorMsg);
//        }
//        return;
//      }
//      codeList[byteIndex] = res;
//      byteIndex++;
//    } else if (elem.is<byte>()) {
//      codeList[byteIndex] = elem.as<byte>();
//      byteIndex++;
//    } else if (elem.is<int>()) {
//      int res = elem.as<int>();
//      if (res > 0xFFFF) {
//        JSONbuffer.clear();
//        char* errorMsg = strdup((const char*)F("Input number exceeds 0xFFFF (65535)"));
//        httpSendIntError(byteIndex, (char *)elem.as<int>(), errorMsg);
//        if (errorMsg) {
//          free(errorMsg);
//        }
//        return;
//      }
//      codeList[byteIndex] = res & 0xFFFF;
//      byteIndex++;
//    } else {
//      JSONbuffer.clear();
//      DynamicJsonDocument JSONErrorBuffer(512);
//      char JSONErrorMessageBuffer[512];
//      
//      JsonObject JSONErrorEncoder = JSONErrorBuffer.to<JsonObject>();
//      JSONErrorEncoder["payload"];
//      JSONErrorEncoder["payload"]["meta"];
//      JSONErrorEncoder["payload"]["status"];
//    
//      JSONErrorEncoder["payload"]["meta"]["uptime"] = millis();
//      JSONErrorEncoder["payload"]["meta"]["heapAvailable"] = heapAvailable();
//      JSONErrorEncoder["payload"]["meta"]["stackAvailable"] = stackAvailable();
//    
//      JSONErrorEncoder["payload"]["status"]["code"] = F("400");
//      JSONErrorEncoder["payload"]["status"]["codeMessage"] = F("Bad Request");
//      JSONErrorEncoder["payload"]["status"][F("errorDescription")] = F("OPs array does not only contain String<byte> or int types.");
//      JSONErrorEncoder["payload"]["status"][F("goodExample")] = F("{'ops': ['0x04', '0x05', 5, 7]}");
//      JSONErrorEncoder["payload"]["status"][F("badExample")] = F("{'ops': [0x04, 0x05, A, B, [1, 2], {'some': 'obj'} ]}");
//
//      serializeJson(JSONErrorEncoder, JSONErrorMessageBuffer);
//      
//      webServer.send(400, "application/json", JSONErrorMessageBuffer);
//      return;
//    }
//  }
//
//  JSONbuffer.clear();
//  
//  char errorMessage[DEFAULT_MESSAGE_SIZE] = {0};
//
//  // Store the string result of each of the commands into an array of strings.
//  char **execRes = (char **) calloc(byteIndex, sizeof(char*));
//  char **echoBack = (char **) calloc(byteIndex, sizeof(char*));
//
//  if (**execRes == NULL) {
//    strcpy(errorMessage, "RAM alloc for **execRes fail");
//    Serial.println(errorMessage);
//    httpSendIntError(errorMessage);
//    return;
//  }
//  
//  if (**execRes == NULL) {
//    strcpy(errorMessage, "RAM alloc for **echoBack fail");
//    httpSendIntError(errorMessage);
//    return;
//  }
//
//  int commandCount = 0;
//  // Basically loops and parses the inputs into commands to execute on the EEPROM memory
//    processCommandInputHttp(codeList, byteIndex, execRes, errorMessage, echoBack, &commandCount); // Function is in serial_http_prog.ino
//
//  if (strlen(errorMessage) > 0) {
//    httpSendIntError(errorMessage);
//    return;
//  }
//
//  DynamicJsonDocument JSONResponseBuffer(256);
//  char JSONResponseMessageBuffer[256];
//
//  JsonObject JSONResponseEncoder = JSONResponseBuffer.to<JsonObject>();
//  JSONResponseEncoder["payload"];
//  JSONResponseEncoder["payload"]["meta"];
//  JSONResponseEncoder["payload"]["status"];
//
//  JSONResponseEncoder["payload"]["meta"]["uptime"] = millis();
//  JSONResponseEncoder["payload"]["meta"]["heapAvailable"] = heapAvailable();
//  JSONResponseEncoder["payload"]["meta"]["stackAvailable"] = stackAvailable();
//
//  JSONResponseEncoder["payload"]["status"]["code"] = "200";
//
//  JsonArray jsonExecRes = JSONResponseEncoder.createNestedArray("execRes");
//  JsonArray jsonEchoBack = JSONResponseEncoder.createNestedArray("echoBack");
//
//  for (int i = 0; i < commandCount; i++) {
//    jsonExecRes.add(execRes[i]);
//    jsonEchoBack.add(echoBack[i]);
//  }
//
//  JSONResponseEncoder["payload"]["status"][F("execRes")] = execRes;
//  JSONResponseEncoder["payload"]["status"][F("echoBack")] = echoBack;
//
//  serializeJson(JSONResponseEncoder, JSONResponseMessageBuffer);
//
//  delay(100); // Give power time to stablise to avoid low power crashes
//  
//  webServer.send(200, "application/json", JSONResponseMessageBuffer);
//  
//  for (int i = 0; i < OP_PROCESS_ARR_LIMIT && i < byteIndex; i++) {
//    if (execRes[i]) {
//      free(execRes[i]);
//    }
//
//    if (echoBack[i]) {
//      free(echoBack[i]);
//    }
//  }
//  
//  if (execRes) {
//    free(execRes);
//  }
//  if (echoBack) {
//    free(echoBack);
//  }
//  
//  delay(50);
}

void ICACHE_FLASH_ATTR httpHandleNotFound() {
  DynamicJsonDocument JSONbuffer(256);
  char JSONmessageBuffer[256];
  
  JsonObject JSONencoder = JSONbuffer.to<JsonObject>();
  JSONencoder["payload"];
  JSONencoder["payload"]["meta"];
  JSONencoder["payload"]["status"];

  JSONencoder["payload"]["meta"]["uptime"] = millis();
  JSONencoder["payload"]["meta"]["heapAvailable"] = heapAvailable();
  JSONencoder["payload"]["meta"]["stackAvailable"] = stackAvailable();

  JSONencoder["payload"]["status"]["code"] = F("404");
  JSONencoder["payload"]["status"]["codeMessage"] = F("Not Found");

  serializeJson(JSONencoder, JSONmessageBuffer);
  
  webServer.send(404, "application/json", JSONmessageBuffer);
}
