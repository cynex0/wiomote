#include "Logger.h"
#include "Command.h"

#include <Arduino.h>

Logger::Logger() {}

void Logger::begin() {
  Serial.begin(BAUD_RATE);
  Serial.println("LOGGING ENABLED");
}

void Logger::log(const char* message) {
  Serial.print(message);
}

void Logger::log(const __FlashStringHelper* message) {
  Serial.print(message);
}

void Logger::log(const int message) {
  Serial.print(message);
}

void Logger::logMqtt(const char* topic, const char* payload, MqttMessageDirection direction) {
  switch (direction) {
    case MqttMessageDirection::OUT:
      Serial.print(F("Published message ["));
      break;
    case MqttMessageDirection::IN:
      Serial.print(F("Message arrived ["));
  }

  Serial.print(topic); 
  Serial.print(F("]: "));
  Serial.println(payload);
}

void Logger::logIR(Command* command) {
  Serial.print(F("Signal sent: [")); Serial.print(command->getDataLength()); Serial.print(F("]{"));

  for (uint8_t i = 0; i < command->getDataLength(); i++) {
    Serial.print(command->getRawData()[i]);

    if (i != command->getDataLength() - 1) {
      Serial.print(F(", "));
    }
  }

  Serial.println(F("}"));
}

void Logger::logWifiConnected(const String &ssid, const String &ip) {
  log(F("Connected to "));
  log(ssid.c_str());
  log(F(". IP address: "));
  log(ip.c_str());
  log("\n");
}
