// Command.h
#ifndef COMMAND
#define COMMAND

struct Command {
  uint16_t *rawData;
  uint8_t dataLength;
  short keyCode;
};

#endif