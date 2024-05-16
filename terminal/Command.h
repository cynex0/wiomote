// Command.h
#ifndef COMMAND_H
#define COMMAND_H

#include <ArduinoJson.h>

#define SERIALIZE_BUF_SIZE 1024

class Command {
private:
  uint16_t *rawData;
  uint8_t dataLength;
  short keyCode; // global keyCode, negative for physical

public:
  Command(const char* jsonString); // create Command instance from a jsonString
  Command(uint16_t *rawData, uint8_t dataLength, short keyCode); // create Command instance by defining all members

  uint16_t* getRawData();
  uint8_t getDataLength();
  short getKeyCode();

  void deserialize(const char* jsonString);
  char* serialize(const char* label);
  JsonDocument serializeToDoc(const char* label);
};

#endif