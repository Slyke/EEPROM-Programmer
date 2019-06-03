
void ICACHE_FLASH_ATTR httpHandleRoot() {
  StaticJsonDocument<256> JSONbuffer;
  char JSONmessageBuffer[256];
  
  JsonObject JSONencoder = JSONbuffer.to<JsonObject>();
  JSONencoder[F("payload")];
  JSONencoder[F("payload")][F("meta")];
  JSONencoder[F("payload")][F("device")];
  JSONencoder[F("payload")][F("data")];

  JSONencoder[F("payload")][F("meta")][F("uptime")] = millis();
  JSONencoder[F("payload")][F("meta")][F("heapAvailable")] = heapAvailable();
  JSONencoder[F("payload")][F("meta")][F("stackAvailable")] = stackAvailable();

  JSONencoder[F("payload")][F("device")][F("ip")] = (String)WiFi.localIP().toString();
  JSONencoder[F("payload")][F("device")][F("rssi")] = (long)WiFi.RSSI();

  serializeJson(JSONencoder, JSONmessageBuffer);
  
  webServer.send(200, "application/json", JSONmessageBuffer);
}

void ICACHE_FLASH_ATTR httpSendIntError(int locIndex, char* errorChars, char* errorMessage) {
  StaticJsonDocument<256> JSONErrorBuffer;
  char JSONErrorMessageBuffer[256];
  
  JsonObject JSONErrorEncoder = JSONErrorBuffer.to<JsonObject>();
  httpEncodeIntError(JSONErrorEncoder, locIndex, errorChars, errorMessage);
  
  serializeJson(JSONErrorEncoder, JSONErrorMessageBuffer);
  webServer.send(400, "application/json", JSONErrorMessageBuffer);
}

void ICACHE_FLASH_ATTR httpSendIntError(char* errorMessage) {
  StaticJsonDocument<256> JSONErrorBuffer;
  char JSONErrorMessageBuffer[256];
  
  JsonObject JSONErrorEncoder = JSONErrorBuffer.to<JsonObject>();

  httpEncodeIntError(JSONErrorEncoder, errorMessage);

  serializeJson(JSONErrorEncoder, JSONErrorMessageBuffer);
  webServer.send(400, "application/json", JSONErrorMessageBuffer);
}

void ICACHE_FLASH_ATTR httpEncodeIntError(const JsonObject &JSONencoder, int locIndex, char* errorChars, char* errorMessage) {
  JSONencoder[F("payload")];
  JSONencoder[F("payload")][F("meta")];
  JSONencoder[F("payload")][F("status")];

  JSONencoder[F("payload")][F("meta")][F("uptime")] = millis();
  JSONencoder[F("payload")][F("meta")][F("heapAvailable")] = heapAvailable();
  JSONencoder[F("payload")][F("meta")][F("stackAvailable")] = stackAvailable();

  JSONencoder[F("payload")][F("status")][F("code")] = F("400");
  JSONencoder[F("payload")][F("status")][F("codeMessage")] = F("Bad Request");
  JSONencoder[F("payload")][F("status")][F("errorDescription")] = errorMessage;
  JSONencoder[F("payload")][F("status")][F("location")] = locIndex;
  JSONencoder[F("payload")][F("status")][F("characters")] = errorChars;
}

void ICACHE_FLASH_ATTR httpEncodeIntError(const JsonObject JSONencoder, char* errorMessage) {
  JSONencoder[F("payload")];
  JSONencoder[F("payload")][F("meta")];
  JSONencoder[F("payload")][F("status")];

  JSONencoder[F("payload")][F("meta")][F("uptime")] = millis();
  JSONencoder[F("payload")][F("meta")][F("heapAvailable")] = heapAvailable();
  JSONencoder[F("payload")][F("meta")][F("stackAvailable")] = stackAvailable();

  JSONencoder[F("payload")][F("status")][F("code")] = F("400");
  JSONencoder[F("payload")][F("status")][F("codeMessage")] = F("Bad Request");
  JSONencoder[F("payload")][F("status")][F("errorDescription")] = errorMessage;
}

void ICACHE_FLASH_ATTR httpHandleInt() {
  if (webServer.hasArg("plain") == false) {
    httpSendIntError("No POST body data.");
    return;
  }

  if (strlen(webServer.arg("plain").c_str()) > OP_PROCESS_ARR_LIMIT) {
    httpSendIntError("POST body too large. Keep smaller than 512 bytes.");
    return;
  }

  DynamicJsonDocument JSONbuffer(512);
  deserializeJson(JSONbuffer, webServer.arg("plain"));

  JsonArray opList = JSONbuffer["ops"];
  char codeList[OP_PROCESS_ARR_LIMIT];
  int byteIndex = 0;

  for(JsonVariant elem: opList) {
    if (byteIndex > OP_PROCESS_ARR_LIMIT) {
      JSONbuffer.clear();
      char* errorMsg = strdup((const char*)F("Byte array length exceeds 512"));
      
      httpSendIntError(errorMsg); // Just outputs a JSON error message
      return;
    }
    if (elem.is<char*>()) {
      unsigned int res;
      char* inputChar = strdup(elem.as<char*>());
      boolean numRes = stringToNumber((int *)&res, inputChar);
      if (!numRes) {
        char* errorMsg = strdup((const char*)F("Invalid character in OP list"));
        httpSendIntError(byteIndex, inputChar, errorMsg);
        return;
      }
      codeList[byteIndex] = res;
      byteIndex++;
    } else if (elem.is<byte>()) {
      codeList[byteIndex] = elem.as<byte>();
      byteIndex++;
    } else if (elem.is<int>()) {
      int res = elem.as<int>();
      if (res > 0xFFFF) {
        char* errorMsg = strdup((const char*)F("Input number exceeds 0xFFFF (65535)"));
        httpSendIntError(byteIndex, (char *)elem.as<int>(), errorMsg);
        return;
      }
      codeList[byteIndex] = res & 0xFFFF;
      byteIndex++;
    } else {
      JSONbuffer.clear();
      StaticJsonDocument<512> JSONErrorBuffer;
      char JSONErrorMessageBuffer[512];
      
      JsonObject JSONErrorEncoder = JSONErrorBuffer.to<JsonObject>();
      JSONErrorEncoder[F("payload")];
      JSONErrorEncoder[F("payload")][F("meta")];
      JSONErrorEncoder[F("payload")][F("status")];
    
      JSONErrorEncoder[F("payload")][F("meta")][F("uptime")] = millis();
      JSONErrorEncoder[F("payload")][F("meta")][F("heapAvailable")] = heapAvailable();
      JSONErrorEncoder[F("payload")][F("meta")][F("stackAvailable")] = stackAvailable();
    
      JSONErrorEncoder[F("payload")][F("status")][F("code")] = F("400");
      JSONErrorEncoder[F("payload")][F("status")][F("codeMessage")] = F("Bad Request");
      JSONErrorEncoder[F("payload")][F("status")][F("errorDescription")] = F("OPs array does not only contain String<byte> or int types.");
      JSONErrorEncoder[F("payload")][F("status")][F("goodExample")] = F("{'ops': ['0x04', '0x05', 5, 7]}");
      JSONErrorEncoder[F("payload")][F("status")][F("badExample")] = F("{'ops': [0x04, 0x05, A, B, [1, 2], {'some': 'obj'} ]}");

      serializeJson(JSONErrorEncoder, JSONErrorMessageBuffer);
      
      webServer.send(400, "application/json", JSONErrorMessageBuffer);
      return;
    }
  }

  JSONbuffer.clear();

  // Store the string result of each of the commands into an array of strings.
  char **execRes = (char **) calloc(byteIndex, sizeof(char*));
  char **echoBack = (char **) calloc(byteIndex, sizeof(char*));
  
  char errorMessage[32] = {0};
  // Basically loops and parses the inputs into commands to execute on the EEPROM memory
  processCommandInputHttp(codeList, byteIndex, execRes, errorMessage, echoBack); // Function is in serial_http_prog.ino
  
  if (strlen(errorMessage) > 0) {
    httpSendIntError(errorMessage);
    return;
  }

  StaticJsonDocument<512> JSONResponseBuffer;
  char JSONResponseMessageBuffer[512];
  
  JsonObject JSONResponseEncoder = JSONResponseBuffer.to<JsonObject>();
  JSONResponseEncoder[F("payload")];
  JSONResponseEncoder[F("payload")][F("meta")];
  JSONResponseEncoder[F("payload")][F("status")];

  JSONResponseEncoder[F("payload")][F("meta")][F("uptime")] = millis();
  JSONResponseEncoder[F("payload")][F("meta")][F("heapAvailable")] = heapAvailable();
  JSONResponseEncoder[F("payload")][F("meta")][F("stackAvailable")] = stackAvailable();

  JSONResponseEncoder[F("payload")][F("status")][F("code")] = F("200");
  JSONResponseEncoder[F("payload")][F("status")][F("execRes[0]")] = execRes[0]; // Test output of the first result.

  serializeJson(JSONResponseEncoder, JSONResponseMessageBuffer);

  delay(100); // Give power time to stablise to avoid low power crashes
  
  webServer.send(200, "application/json", JSONResponseMessageBuffer);
  
  for (int i = 0; i < OP_PROCESS_ARR_LIMIT && i < byteIndex; i++) {
    if (execRes[i]) {
      free(execRes[i]);
    }

    if (echoBack[i]) {
      free(echoBack[i]);
    }
  }
  
  if (execRes) {
    free(execRes);
  }
  if (echoBack) {
    free(echoBack);
  }
  
  delay(50);
}

void ICACHE_FLASH_ATTR httpHandleNotFound() {
  StaticJsonDocument<256> JSONbuffer;
  char JSONmessageBuffer[256];
  
  JsonObject JSONencoder = JSONbuffer.to<JsonObject>();
  JSONencoder[F("payload")];
  JSONencoder[F("payload")][F("meta")];
  JSONencoder[F("payload")][F("status")];

  JSONencoder[F("payload")][F("meta")][F("uptime")] = millis();
  JSONencoder[F("payload")][F("meta")][F("heapAvailable")] = heapAvailable();
  JSONencoder[F("payload")][F("meta")][F("stackAvailable")] = stackAvailable();

  JSONencoder[F("payload")][F("status")][F("code")] = F("404");
  JSONencoder[F("payload")][F("status")][F("codeMessage")] = F("Not Found");

  serializeJson(JSONencoder, JSONmessageBuffer);
  
  webServer.send(404, "application/json", JSONmessageBuffer);
}
