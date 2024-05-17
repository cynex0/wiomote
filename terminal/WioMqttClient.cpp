#include "WioMqttClient.h"
#include "Logger.h"

// Create a client without logging
WioMqttClient::WioMqttClient(WiFiClient wifiClient_, MqttCallback mqttCallback_): 
  wifiClient(wifiClient_),
  mqttClient(wifiClient),
  callback(mqttCallback_), 
  logger(nullptr),
  loggingEnabled(false),
  lastPinged(0) 
{}


// Create a client with logging
WioMqttClient::WioMqttClient(WiFiClient wifiClient_, MqttCallback mqttCallback_, Logger* logger_):
  wifiClient(wifiClient_),
  mqttClient(wifiClient),
  callback(mqttCallback_), 
  logger(logger_),
  loggingEnabled(true),
  lastPinged(0)
{}


void WioMqttClient::setup() {
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(callback);
  mqttClient.setBufferSize(MSG_BUFFER_SIZE);
}

// Modification of: https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_basic/mqtt_basic.ino
void WioMqttClient::update() {
  if (mqttClient.connected()) {
    mqttClient.loop();

    if(millis() - lastPinged > MQTT_PING_INTERVAL) {
      publishWithLog(TOPIC_CONN_OUT, "ping");
      lastPinged = millis();
    }

  } 
  else {
    if (loggingEnabled)
      logger->log(F("Attempting MQTT connection...\n"));
    
    
    // Create a random client ID so that it does 
    // not clash with other subscribed clients
    const String clientId = UUID_PREFIX + String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str())) {
      if (loggingEnabled) {
        logger->log(F("Connected to MQTT server.\n"));
      }

      mqttClient.subscribe(TOPIC_IR_IN); // topic to receive IR commands from the app
      mqttClient.subscribe(TOPIC_SWITCH_MODE); // topic to switch to receiveMode when creating custom buttons in the app 
    } 
    else {
      if (loggingEnabled)
        logger->log(F("Failed to connect to MQTT server - rc="));
        logger->log(mqttClient.state());
        logger->log("\n");
    }
  }
}


void WioMqttClient::publishWithLog(const char* topic, const char* payload) {
  mqttClient.publish(topic, payload);
  if (logger != nullptr) {
    logger->logMqtt(topic, payload, MqttMessageDirection::OUT);
  }
}