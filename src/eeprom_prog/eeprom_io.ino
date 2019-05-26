void ICACHE_FLASH_ATTR exEepromWriteByte(int deviceAddress, unsigned int memAddress, byte data) {
  int rdata = data;
  delay(25);
  Wire.beginTransmission(deviceAddress);
  Wire.write((int)(memAddress >> 8)); // MSB
  Wire.write((int)(memAddress & 0xFF)); // LSB
  Wire.write(rdata & 0xFF);
  Wire.endTransmission();
  delay(25);
}

byte ICACHE_FLASH_ATTR exEepromReadByte(byte* res, int deviceAddress, unsigned int memAddress, byte defaultValue) {
  *res = defaultValue;
  delay(25);
  Wire.beginTransmission(deviceAddress);
  Wire.write((int)(memAddress >> 8)); // MSB
  Wire.write((int)(memAddress & 0xFF)); // LSB
  Wire.endTransmission();
  Wire.requestFrom(deviceAddress, 1);
  if (Wire.available()) {
    *res = Wire.read();
    delay(25);
    return 0;
  }
  delay(25);
  return 1;
}

byte ICACHE_FLASH_ATTR readStringFromEeprom(String* res, int deviceAddress, unsigned int startAddress, byte stringMaxLength) {
  byte stringIndex = 0;
  *res = "";
  char buf;
  while(stringIndex < stringMaxLength) {
    byte readResult = exEepromReadByte((byte *)&buf, deviceAddress, startAddress + stringIndex, EEPROM_READ_FAILURE);
    String temp(buf);
    if (buf == 0) {
      return 0;
    }
    if (buf < 32 || buf > 126 || readResult > 0) {
      return 1;
    }
    res->concat(buf);
    stringIndex++;
  }
  return 0;
}

byte ICACHE_FLASH_ATTR exEepromReadByte(byte* res, int deviceAddress, unsigned int memAddress) {
  return exEepromReadByte(res, deviceAddress, memAddress, (byte)EEPROM_READ_FAILURE);
}
