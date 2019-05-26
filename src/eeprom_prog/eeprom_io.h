#ifndef EEPROM_IO_H
#define EEPROM_IO_H

void ICACHE_FLASH_ATTR exEepromWriteByte(int deviceAddress, unsigned int memAddress, byte data);
byte ICACHE_FLASH_ATTR exEepromReadByte(byte* res, int deviceAddress, unsigned int memAddress, byte defaultValue);
byte ICACHE_FLASH_ATTR readStringFromEeprom(String* res, int deviceAddress, unsigned int startAddress, byte stringMaxLength);
byte ICACHE_FLASH_ATTR exEepromReadByte(byte* res, int deviceAddress, unsigned int memAddress);

#endif
