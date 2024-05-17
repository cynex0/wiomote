#include "Command.h"
#include <ArduinoJson.h>

Command::Command():
  rawData(new uint16_t[0]),
  dataLength(0),
  keyCode(0)
{}

Command::Command(const char* jsonString):
  rawData(new uint16_t[0]),
  dataLength(0),
  keyCode(0) 
{
  deserialize(jsonString);
}

Command::Command(uint16_t *rawData_, uint8_t dataLength_, short keyCode_):
  rawData(rawData_),
  dataLength(dataLength_),
  keyCode(keyCode_)
{}

uint16_t* Command::getRawData() {
  return rawData;
}

uint8_t Command::getDataLength() {
  return dataLength;
}

short Command::getKeyCode() {
  return keyCode;
}

void Command::deserialize(const char* jsonString) {
  JsonDocument* doc = new JsonDocument;
  deserializeJson(*doc, jsonString);
  
  if (dataLength != 0) delete[] rawData; // Free previously used memory

  keyCode = (*doc)[F("keyCode")]; 
  dataLength = (*doc)[F("command")][F("dataLength")];
  
  JsonArray rawDataJson = (*doc)[F("command")][F("rawData")];
  rawData = new uint16_t[dataLength];
  for (uint8_t i = 0; i < dataLength; i++) {
    rawData[i] = rawDataJson[i];
  }

  delete doc;
}

JsonDocument Command::serializeToDoc(const char* label) {
  JsonDocument doc;
  JsonObject commandObj = doc.createNestedObject("command");

  doc["keyCode"] = keyCode;
  commandObj["label"] = label;
  commandObj["dataLength"] = dataLength;

  JsonArray rawDataJson = commandObj.createNestedArray("rawData");
  for (uint8_t i = 0; i < dataLength; i++) {
    rawDataJson.add(rawData[i]);
  }

  return doc;
}

/* JSON format (may also contain more keys):
  {
    "keyCode":<keyCode>,
    "command":{
      "label":<label>
      "dataLength":<length>,
      "rawData":[<byte0>,<byte1>,...]
    }
  }
*/
char* Command::serialize(const char* label) {
  JsonDocument* doc = new JsonDocument;
  JsonObject commandObj = doc->createNestedObject("command");

  (*doc)["keyCode"] = keyCode;
  commandObj["label"] = label;
  commandObj["dataLength"] = dataLength;

  JsonArray rawDataJson = commandObj.createNestedArray("rawData");
  for (uint8_t i = 0; i < dataLength; i++) {
	  rawDataJson.add(rawData[i]);
  }

  char* out = new char[SERIALIZE_BUF_SIZE]; // Buffer to hold the output
  serializeJson(*doc, out, SERIALIZE_BUF_SIZE);

  delete doc;
  return out;
}

Command::~Command() {
  delete[] rawData; // free dynamically allocated memory
}