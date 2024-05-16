#include "WioMqttClient.h"

WioMqttClient::WioMqttClient(WiFiClient wifiClient_, MqttCallback mqttCallback_): 
  // initialize members via initializer list
  wifiClient(wifiClient_),
  mqttClient(wifiClient),
  callback(mqttCallback_), 
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
    #ifdef MQTT_PING
      if(millis() - lastPinged > MQTT_PING_INTERVAL) {
        publishWithLog(TOPIC_CONN_OUT, "ping");
        lastPinged = millis();
      }
    #endif
  } else {
    #ifdef DEBUG_LOG
      Serial.println(F("Attempting MQTT connection..."));
    #endif
    
    // Create a random client ID so that it does 
    // not clash with other subscribed clients
    const String clientId = UUID_PREFIX + String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str())) {
      #ifdef DEBUG_LOG
        Serial.println(F("Connected to MQTT server. Publishing a test message."));
        mqttPublishWithLog(TOPIC_CONN_OUT, "Publish test WIO");
      #endif

      mqttClient.subscribe(TOPIC_IR_IN); // topic to receive IR commands from the app
      mqttClient.subscribe(TOPIC_SWITCH_MODE); // topic to switch to receiveMode when creating custom buttons in the app 
    } else {
      #ifdef DEBUG_LOG
        Serial.print(F("Failed to connect to MQTT server - rc=" + client.state()));
      #endif
    }
  }
}


void WioMqttClient::publishWithLog(const char* topic, const char* payload) {
  mqttClient.publish(topic, payload);
  #ifdef DEBUG_LOG
    Serial.print(F("Published message [")); Serial.print(topic); Serial.print(F("]: "));
    Serial.println(payload);
  #endif
}