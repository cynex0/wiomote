// WioMqttClient.h
#ifndef WIO_MQTT // prevent importing more than once
#define WIO_MQTT


#define MQTT_PING  // send "ping"s

#ifdef MQTT_PING
#define MQTT_PING_INTERVAL 1000 // interval between pings
#endif

#include <PubSubClient.h>
#include <rpcWiFi.h>

// Terminal commands to check if it works via Mosquitto
// mosquitto_sub -v -h 'broker.hivemq.com' -p 1883 -t 'dit113/testwio12321Out'
// mosquitto_pub -h 'broker.hivemq.com' -p 1883 -t "dit113/testwio12321In" -m "message to terminal"

// Constants
#define MQTT_SERVER "broker.hivemq.com"
#define MQTT_PORT                 1883
#define UUID_PREFIX     "WioTerminal-"
#define MSG_BUFFER_SIZE           8192

#define TOPIC_CONN_OUT "wiomote/connection/terminal"
#define TOPIC_IR_IN                 "wiomote/ir/app"
#define TOPIC_IR_OUT           "wiomote/ir/terminal"
#define TOPIC_CURRENT_MODE            "wiomote/mode"
#define TOPIC_SWITCH_MODE     "wiomote/mode/request"

typedef void (*MqttCallback)(char*, byte*, unsigned int); // custom type for the callback method

class WioMqttClient {
private:
  WiFiClient wifiClient; // wifiClient reference needs to be stored for PubSubClient to function
  PubSubClient mqttClient;
  MqttCallback callback; // pointer to the callback method, automatically executed when a message is received
  unsigned long lastPinged;

public:
  WioMqttClient(WiFiClient wifiClient, MqttCallback mqttCallback);
  void setup();
  void update();
  void publishWithLog(const char* topic, const char* payload);
};

#endif