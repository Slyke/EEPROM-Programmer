
byte ICACHE_FLASH_ATTR decodeKeypad() {
  static bool noPress = 0;
  
  for(byte i = 0; i < pinCols; i++) {
    if (digitalRead(inputPins[i]) == HIGH);
    else
    break;
    if(i == (pinCols - 1)) {
      noPress = 1;
      scanR = 0;
      scanC = 0;
    }
  }
  
  if(noPress == 1) {
    for(byte i = 0; i < pinRows; i++) {
      digitalWrite(outputPins[i], LOW);
    }
    
    for(byte i = 0; i < pinCols; i++) {
      if(digitalRead(inputPins[i]) == HIGH) {
        continue;
      } else {
        for (scanR = 0; scanR < pinRows; scanR++) {
          digitalWrite(outputPins[scanR], HIGH);   
          if(digitalRead(inputPins[i]) == HIGH) {
            noPress = 0;               
            for(scanC = 0; scanC < pinRows; scanC++) {
              digitalWrite(outputPins[scanC], LOW);
            }
            return scanR * pinRows + i;  
          }
        }
      }
    }
  }

 return NO_KEY;
}

void ICACHE_FLASH_ATTR setupKeypadMatrixPins() {
  return;
  for(byte i = 0; i < pinRows; i++) {
    pinMode(outputPins[i],OUTPUT);
  }
  for(byte j = 0; j < pinCols; j++) {
    pinMode(inputPins[j],INPUT_PULLUP);
  }
}
