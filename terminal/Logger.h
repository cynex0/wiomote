#ifndef LOGGER_H
#define LOGGER_H

#define BAUD_RATE 9600

#include "Command.h"

enum class MqttMessageDirection {IN, OUT};

class Logger {
public:
  Logger();
  void begin();
  void log(const char*);
  void log(const __FlashStringHelper*);
  void log(const int);
  void logMqtt(const char*, const char*, MqttMessageDirection);
  void logIR(Command*);
  void logWifiConnected(const String&, const String&);
};

#endif