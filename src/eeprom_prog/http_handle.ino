
void ICACHE_FLASH_ATTR httpHandleRoot() {
  DynamicJsonDocument JSONbuffer(1024);
  char JSONmessageBuffer[1024];
  
  JsonObject JSONencoder = JSONbuffer.to<JsonObject>();
  JSONencoder[F("payload")];
  JSONencoder[F("payload")][F("meta")];
  JSONencoder[F("payload")][F("data")];

  JSONencoder[F("payload")][F("meta")][F("uptime")] = millis();

  serializeJson(JSONencoder, JSONmessageBuffer);
  
  webServer.send(200, "application/json", JSONmessageBuffer);
}

void ICACHE_FLASH_ATTR httpHandleIntError(int locIndex, char* errorChars, char* errorMessage) {
    StaticJsonDocument<512> JSONbuffer;
    char JSONmessageBuffer[512];
    
    JsonObject JSONencoder = JSONbuffer.to<JsonObject>();
    JSONencoder[F("payload")];
    JSONencoder[F("payload")][F("meta")];
    JSONencoder[F("payload")][F("status")];
  
    JSONencoder[F("payload")][F("meta")][F("uptime")] = millis();
  
    JSONencoder[F("payload")][F("status")][F("code")] = F("400");
    JSONencoder[F("payload")][F("status")][F("codeMessage")] = F("Bad Request");
    JSONencoder[F("payload")][F("status")][F("errorDescription")] = errorMessage;
    JSONencoder[F("payload")][F("status")][F("location")] = locIndex;
    JSONencoder[F("payload")][F("status")][F("characters")] = errorChars;
    
    serializeJson(JSONencoder, JSONmessageBuffer);
    
    webServer.send(400, "application/json", JSONmessageBuffer);
}

void ICACHE_FLASH_ATTR httpHandleInt() {
  if (webServer.hasArg("plain") == false) {
    StaticJsonDocument<256> JSONbuffer;
    char JSONmessageBuffer[256];
    
    JsonObject JSONencoder = JSONbuffer.to<JsonObject>();
    JSONencoder[F("payload")];
    JSONencoder[F("payload")][F("meta")];
    JSONencoder[F("payload")][F("status")];
  
    JSONencoder[F("payload")][F("meta")][F("uptime")] = millis();
  
    JSONencoder[F("payload")][F("status")][F("code")] = F("400");
    JSONencoder[F("payload")][F("status")][F("codeMessage")] = F("Bad Request");
  
    serializeJson(JSONencoder, JSONmessageBuffer);
    
    webServer.send(400, "application/json", JSONmessageBuffer);
    return;
  }

  DynamicJsonDocument JSONbuffer(1024);
  deserializeJson(JSONbuffer, webServer.arg("plain"));

  JsonArray opList = JSONbuffer["ops"];
  byte codeList[OP_PROCESS_ARR_LIMIT];
  int byteIndex = 0;

  for(JsonVariant elem: opList) {
    if (byteIndex > OP_PROCESS_ARR_LIMIT) {
      char* errorMsg = strdup((const char*)F("Byte array length exceeds 512"));
      httpHandleIntError(byteIndex, (char *)elem.as<int>(), errorMsg);
      return;
    }
    if (elem.is<char*>()) {
      unsigned int res;
      char* inputChar = strdup(elem.as<char*>());
      boolean numRes = stringToNumber((int *)&res, inputChar);
      if (!numRes) {
        char* errorMsg = strdup((const char*)F("Invalid character in OP list"));
        httpHandleIntError(byteIndex, inputChar, errorMsg);
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
        httpHandleIntError(byteIndex, (char *)elem.as<int>(), errorMsg);
        return;
      }
      codeList[byteIndex] = res & 0xFFFF;
      byteIndex++;
    } else {
      StaticJsonDocument<512> JSONbuffer;
      char JSONmessageBuffer[512];
      
      JsonObject JSONencoder = JSONbuffer.to<JsonObject>();
      JSONencoder[F("payload")];
      JSONencoder[F("payload")][F("meta")];
      JSONencoder[F("payload")][F("status")];
    
      JSONencoder[F("payload")][F("meta")][F("uptime")] = millis();
    
      JSONencoder[F("payload")][F("status")][F("code")] = F("400");
      JSONencoder[F("payload")][F("status")][F("codeMessage")] = F("Bad Request");
      JSONencoder[F("payload")][F("status")][F("errorDescription")] = F("OPs array does not only contain String<byte> or int types.");
      JSONencoder[F("payload")][F("status")][F("goodExample")] = F("{'ops': ['0x04', '0x05', 5, 7]}");
      JSONencoder[F("payload")][F("status")][F("badExample")] = F("{'ops': [0x04, 0x05, A, B, [1, 2], {'some': 'obj'} ]}");
    
      serializeJson(JSONencoder, JSONmessageBuffer);
      
      webServer.send(400, "application/json", JSONmessageBuffer);
      return;
    }

    
  }

//  char  

  webServer.send(200, "application/json", "hi");
}

void ICACHE_FLASH_ATTR httpHandleNotFound() {
  StaticJsonDocument<256> JSONbuffer;
  char JSONmessageBuffer[256];
  
  JsonObject JSONencoder = JSONbuffer.to<JsonObject>();
  JSONencoder[F("payload")];
  JSONencoder[F("payload")][F("meta")];
  JSONencoder[F("payload")][F("status")];

  JSONencoder[F("payload")][F("meta")][F("uptime")] = millis();

  JSONencoder[F("payload")][F("status")][F("code")] = F("404");
  JSONencoder[F("payload")][F("status")][F("codeMessage")] = F("Not Found");

  serializeJson(JSONencoder, JSONmessageBuffer);
  
  webServer.send(404, "application/json", JSONmessageBuffer);
}
