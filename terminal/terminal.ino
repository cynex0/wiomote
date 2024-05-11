#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include <rpcWiFi.h>

#include <PubSubClient.h>

#include <IRLibSendBase.h>
#include <IRLib_HashRaw.h>
#include <IRLibCombo.h>
#include <IRLibRecvPCI.h>

#include <TFT_eSPI.h>

// Debugging modes
//#define DEBUG_UI    // additional UI elements
#define DEBUG_LOG   // log events to serial
#define MQTT_PING  // send "ping"s and receive "pong"s

// Button indexes for the array acting as a map
#define BTN_COUNT       6
#define POWER_BTN_INDEX 0 // -1 in the app
#define UP_BTN_INDEX    1 // -2 in the app
#define RIGHT_BTN_INDEX 2 // -3 in the app
#define DOWN_BTN_INDEX  3 // -4 in the app
#define LEFT_BTN_INDEX  4 // -5 in the app
#define PRESS_BTN_INDEX 5 // -6 in the app

// Motor pin
#define MO_PIN D0

// Buzzer pin
#define BUZZER_PIN WIO_BUZZER 

// Buzzer constants
#define BUZZER_FRQ 128 // Buzzer PWM frequency

// UI elements
#define CIRCLE_COLOR        TFT_BLUE
#define OUTER_CIRCLE_COLOR TFT_WHITE
#define CIRCLE_RADIUS             45
#define CENTER_X                 160  // Middle point of screen X-axis
#define CENTER_Y                 120  // Middle point of screen Y-axis
#define SCREEN_ROTATION            3

// Constants for signal icon
#define SIGNAL_ICON_X          280  // X placement of icon
#define SIGNAL_ICON_Y          200  // Y placement of icon
#define ICON_INNER_RADIUS        5  // Radius of the smallest cirlce
#define ICON_OUTER_RADIUS       30  // Radius of the largest circle
#define ICON_RING_SPACING        5  // Space between every ring in icon
#define ICON_SIGNAL_COLOR TFT_BLUE  // Color of the moving signal rings

#define ARROW_TOP_OFFSET  100  // Distance from middle to the top of the arrows
#define ARROW_BASE_OFFSET  60  // Distance from middle to bottom sides of arrows
#define ARROW_LENGTH       40  // Value of arrow length
#define ARROW_COLOR TFT_WHITE

#define TEXT_SIZE_L 3
#define TEXT_SIZE_M 2
#define TEXT_SIZE_S 1

#define ICON_COLOR      TFT_LIGHTGREY  // Define color for connection icon
#define DEFAULT_TEXT_COLOR  TFT_WHITE  // Default text color on dark bg
#define INVERTED_TEXT_COLOR TFT_BLACK  // Inverted text color for light bg

#define DEFAULT_BG_COLOR   TFT_BLACK   // Define standard background color
#define INVERTED_BG_COLOR  TFT_WHITE   // Inverted background color


// Buttons
#define UP_BTN       WIO_5S_UP
#define DOWN_BTN   WIO_5S_DOWN
#define LEFT_BTN   WIO_5S_LEFT
#define RIGHT_BTN WIO_5S_RIGHT
#define PRESS_BTN WIO_5S_PRESS
#define POWER_BTN    WIO_KEY_C
#define MODE_BTN     WIO_KEY_A

// Bluetooth
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

// WiFi
#define CONNECTED    0
#define CONNECTING   1
#define DISCONNECTED 2

// MQTT
#define MQTT_SERVER "broker.hivemq.com"
#define MQTT_PORT                 1883
#define UUID_PREFIX     "WioTerminal-"

#define TOPIC_CONN_OUT  "wiomote/connection/terminal"
#define TOPIC_CONN_IN        "wiomote/connection/app"
#define TOPIC_IR_IN                  "wiomote/ir/app"
#define TOPIC_IR_OUT            "wiomote/ir/terminal"
#define TOPIC_CURRENT_MODE     "wiomote/mode/current"
#define TOPIC_SWITCH_MODE "wiomote/mode/requestClone"

// IR
#define CARRIER_FREQUENCY_KHZ 38

// Wifi connection icon
#define WIFI_CONNECTION_CIRCLE_X                290
#define WIFI_CONNECTION_CIRCLE_Y                 35
#define WIFI_CONNECTION_CIRCLE_MAX_RAD           21
#define WIFI_CONNECTION_CIRCLE_RADIUS_DIFF        7
#define WIFI_CONNECTION_ICON_COLOR_ON      TFT_CYAN
#define WIFI_CONNECTION_ICON_COLOR_OFF TFT_DARKGREY

// Bluetooth connection icon
#define BLT_ICON_START_X            245
#define BLT_ICON_START_Y             11
#define BLT_ICON_WIDTH               15
#define BLT_ICON_HEIGHT              25
#define BLT_ICON_COLOR_ON      TFT_CYAN
#define BLT_ICON_COLOR_OFF TFT_DARKGREY

struct Command {
  uint16_t *rawData;
  uint8_t dataLength;
  short keyCode;
};

// Button map to commands
Command *commandMap = new Command[BTN_COUNT];

// LCD 
TFT_eSPI tft;

// IR emitter and receiver
IRsendRaw emitter;
IRrecvPCI receiver(BCM3);

// WiFi variables
WiFiClient wifiClient;
JsonDocument wifiInfo;
uint8_t wifiDeviceConnected;

// BLE variables
BLEServer *bleServer;
BLECharacteristic *pTxCharacteristic;
bool bleDeviceConnected;
bool bleOldDeviceConnected;

// MQTT variables
PubSubClient mqttClient(wifiClient);
unsigned long lastPinged = 0;

// Logic variables
bool receiveMode = false;
bool prevModeBtnState = HIGH;
int chosenButton = -1; // Button selection in the cloning mode
bool chosenFromApp = false;
bool mappingToCustomButton = false;
bool wifiConnectedPrevVal = true;
bool bltConnectedPrevVal = false;

void decideBltConnectionIcon();

// Motor variables
const int vibDuration = 200;
unsigned long lastVibrated = 0;
bool isVibrating = false;

// Buzzer variables
const int buzzDuration = 400;
unsigned long lastBuzzed = 0;
bool isBuzzing = false;

class BluetoothServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* bleServer) {
      #ifdef DEBUG_LOG
        Serial.println(F("Bluetooth connected"));
      #endif

      bleDeviceConnected = true;
      decideBltConnectionIcon();
      bltConnectedPrevVal = true;
    }

    void onDisconnect(BLEServer* bleServer) {
      #ifdef DEBUG_LOG
        Serial.println(F("Bluetooth disconnected"));
      #endif

      bleDeviceConnected = false;
      decideBltConnectionIcon();
      bltConnectedPrevVal = false;
    }
};

class BluetoothCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *characteristic) {
      std::string rxValue = characteristic -> getValue();

      const char* data = rxValue.c_str();
  
      wifiInfo.clear();
      deserializeJson(wifiInfo, data);

      mqttClient.unsubscribe(TOPIC_CONN_IN);
      WiFi.disconnect();

      wifiDeviceConnected = DISCONNECTED;

      #ifdef DEBUG_LOG
        Serial.println(data);
        Serial.println(F("Cleared existing connections"));
      #endif
    }
};

// Modification of: https://hackmd.io/@amebaiot/H1uRHufYv#Code
void setupBLE() {
  BLEDevice::init("WIOmote");  // Define device name

  bool bleDeviceConnected = false;
  bool bleOldDeviceConnected = false;

  // Create the BLE Server
  bleServer = BLEDevice::createServer();
  bleServer -> setCallbacks(new BluetoothServerCallbacks());

  // Create the BLE Service
  BLEService *bleService = bleServer -> createService(SERVICE_UUID);

  // Create a BLE Characteristic
  BLECharacteristic *rxCharacteristic = bleService->createCharacteristic(
                              CHARACTERISTIC_UUID_RX,
                              BLECharacteristic::PROPERTY_WRITE);
  rxCharacteristic -> setAccessPermissions(GATT_PERM_READ | GATT_PERM_WRITE);           
  rxCharacteristic -> setCallbacks(new BluetoothCallbacks());

  // Start the service
  bleService -> start();

  // Start advertising
  bleServer -> startAdvertising();
}

void mqttPublishWithLog(const char* topic, const char* payload) {
  mqttClient.publish(topic, payload);
  #ifdef DEBUG_LOG
    Serial.print(F("Published message [")); Serial.print(topic); Serial.print(F("]: "));
    Serial.println(payload);
  #endif
}

// Received an MQTT message
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char* buff_p = new char[length];
  for (int i = 0; i < length; i++) {
    buff_p[i] = (char) payload[i];
  }
  buff_p[length] = '\0';


  #ifdef DEBUG_LOG
    Serial.print(F("Message arrived [")); Serial.print(F(topic)); Serial.print(F("] "));
    Serial.println(buff_p);
  #endif
  #ifdef DEBUG_UI
    drawRemote();
    tft.setTextSize(TEXT_SIZE_S);
    tft.setCursor(0, TFT_WIDTH - 2); // width is height :)
    tft.print(F("MQTT: ")); tft.print(F(buff_p));
  #endif

  // A command sent from the app
  if (strcmp(topic, TOPIC_IR_IN) == 0) {
    Command command = deserializeCommand(buff_p);

    if (!receiveMode) {
      emitData(command);
    } else {
      chosenButton = -1 * (command.keyCode + 1);
      chosenFromApp = true;
    }
  }
  else if (strcmp(topic, TOPIC_SWITCH_MODE) == 0) {
    if (!receiveMode) {
      switchMode();
    }
    chosenButton = atoi(buff_p);
    chosenFromApp = true;
    mappingToCustomButton = true;
  }
}

void setupMQTT() {
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(8192);
}

// Terminal commands to check if it works via Mosquitto
// mosquitto_sub -v -h 'broker.hivemq.com' -p 1883 -t 'dit113/testwio12321Out'
// mosquitto_pub -h 'broker.hivemq.com' -p 1883 -t "dit113/testwio12321In" -m "message to terminal"

// Modification of: https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_basic/mqtt_basic.ino
void updateMQTT() {
  if (mqttClient.connected()) {
    mqttClient.loop();
    #ifdef MQTT_PING
      if(millis() - lastPinged > 5000) {
        mqttPublishWithLog(TOPIC_CONN_OUT, "ping");
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

      mqttClient.subscribe(TOPIC_CONN_IN); // topic to receive "pongs" from the app
      mqttClient.subscribe(TOPIC_IR_IN); // topic to receive IR commands from the app
      mqttClient.subscribe(TOPIC_SWITCH_MODE); // topic to switch to receiveMode when creating custom buttons in the app 
    } else {
      #ifdef DEBUG_LOG
        Serial.print(F("Failed to connect to MQTT server - rc=" + mqttClient.state()));
      #endif
    }
  }
}

void WiFiEvent(const WiFiEvent_t event){
  if(event == SYSTEM_EVENT_STA_DISCONNECTED) {
    wifiDeviceConnected = DISCONNECTED;
  }
}

void setupWiFi() {
  // Set WiFi to station mode
  WiFi.mode(WIFI_STA);
  WiFi.onEvent(WiFiEvent);

  wifiDeviceConnected = WiFi.isConnected() ? CONNECTED : DISCONNECTED;
}

uint8_t* stringToMAC(const char* str) {
  char buffer[20];
  uint8_t mac[6];

  strcpy(buffer, str);

  char *tok = strtok(buffer, ":");

  for (int i = 5; i >= 0; i--) {
    mac[i] = strtol(tok, NULL, 16);
    
    tok = strtok(NULL, ":");
  }

  return mac;
}

void updateNetwork() {
  if (!bleDeviceConnected && bleOldDeviceConnected) {
    delay(500); // Give the bluetooth stack the chance to get things ready

    bleServer -> startAdvertising();
    bleOldDeviceConnected = bleDeviceConnected;
  }

  if (bleDeviceConnected && !bleOldDeviceConnected) {
    bleOldDeviceConnected = bleDeviceConnected;
  }
  
  if(wifiInfo.isNull()) {
    return;
  }

  if(WiFi.isConnected()) {
    #ifdef DEBUG_LOG
      if(wifiDeviceConnected != CONNECTED) {
        Serial.print(F("Connected to "));
		    Serial.println(WiFi.SSID());
        Serial.print(F("IP address: "));
        Serial.println(WiFi.localIP());
      }
    #endif

    wifiDeviceConnected = CONNECTED;

	decideWiFiConnectionIcon();
    wifiConnectedPrevVal = true;

    updateMQTT();
  } 
  else {
    const char *ssid = wifiInfo["ssid"];

    if(wifiDeviceConnected != CONNECTING) {  
      #ifdef DEBUG_LOG
        Serial.print(F("Connecting to "));
        Serial.println(F(ssid));
      #endif

      wifiDeviceConnected = CONNECTING;

	  decideWiFiConnectionIcon();
      wifiConnectedPrevVal = false;

      WiFi.begin(ssid, wifiInfo["password"], 0L, stringToMAC(wifiInfo["bssid"]));
    }
  }
}

int getButtonPressedIndex(){
  int out = -1;

  if (digitalRead(PRESS_BTN) == LOW) {
    out = PRESS_BTN_INDEX;
  } else if (digitalRead(UP_BTN) == LOW) {
    out = UP_BTN_INDEX;
  } else if (digitalRead(DOWN_BTN) == LOW) {
    out = DOWN_BTN_INDEX;
  } else if (digitalRead(LEFT_BTN) == LOW) {
    out = LEFT_BTN_INDEX;
  } else if (digitalRead(RIGHT_BTN) == LOW) {
    out = RIGHT_BTN_INDEX;
  } else if (digitalRead(POWER_BTN) == LOW) {
    out = POWER_BTN_INDEX;
  }

  return out;
}

const char* getButtonName(const int index) {
  char* out;
  switch(index) {
    case POWER_BTN_INDEX:
      out = "POWER"; 
      break;
    case UP_BTN_INDEX:
      out = "UP";
      break;
    case RIGHT_BTN_INDEX:
      out = "RIGHT";
      break;
    case DOWN_BTN_INDEX:
      out = "DOWN";
      break;
    case LEFT_BTN_INDEX:
      out = "LEFT";
      break;
    case PRESS_BTN_INDEX:
      out = "OK";
      break;
  }
  return out;
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
char* serializeCommand(const Command& command) {
  JsonDocument* doc = new JsonDocument;
  JsonObject commandObj = doc->createNestedObject("command");

  (*doc)["keyCode"] = command.keyCode;
  commandObj["label"] = getButtonName(-1 * (command.keyCode + 1));
  commandObj["dataLength"] = command.dataLength;

  JsonArray rawDataJson = commandObj.createNestedArray("rawData");
  for (uint8_t i = 0; i < command.dataLength; i++) {
	  rawDataJson.add(command.rawData[i]);
  }

  char* out = new char[1024]; // Buffer to hold the output
  serializeJson(*doc, out, 1024);

  delete doc;
  return out;
}

/* Expected format (may also contain more keys):
  {
    "keyCode":<keyCode>,
    "command":{
      "dataLength":<length>,
      "rawData":[<byte0>,<byte1>,...]
    }
  }
*/
Command deserializeCommand(const char* jsonString) {
  JsonDocument* doc = new JsonDocument;
  deserializeJson(*doc, jsonString);
  
  short keyCode = (*doc)[F("keyCode")]; 

  const uint8_t dataLength = (*doc)[F("command")][F("dataLength")];
  JsonArray rawDataJson = (*doc)[F("command")][F("rawData")];

  uint16_t *rawData = new uint16_t[dataLength];
  for (uint8_t i = 0; i < dataLength; i++) {
    rawData[i] = rawDataJson[i];
  }

  delete doc;
  return {rawData, dataLength, keyCode};
}

void startBuzzer() {
  if (!(isBuzzing)) {  // Checks that buzzer isnt active already
    
    analogWrite(BUZZER_PIN, BUZZER_FRQ); // Start buzzer
    lastBuzzed = millis();          // Log the time of activation
    isBuzzing = true;               // Flag that buzzer is active
  }
}

void updateBuzzer () { // Turns off buzzer after set duration
  
  if (isBuzzing && (millis() - lastBuzzed >= buzzDuration)) { // Check if duration has passed

    analogWrite(BUZZER_PIN, 0);
    isBuzzing = false;
  }
}

void startVibration() {

  if (!(isVibrating)) {  // Ensures a vibration isnt already triggered
    
    digitalWrite(MO_PIN, HIGH); // Start vibration
    lastVibrated = millis();    // Log the time of activation
    isVibrating = true;         // Flag for active vibration
  }
}

void updateVibration() { // Turns off vibration after set duration
  
  if (isVibrating && (millis() - lastVibrated >= vibDuration)) { // Check if duration has passed

    digitalWrite(MO_PIN, LOW);
    isVibrating = false;
  }
}

void drawReceiveSignal() {  // Draw circles for incomming signal

  for (int radius = ICON_OUTER_RADIUS; radius >= ICON_INNER_RADIUS; radius -= ICON_RING_SPACING) {

    tft.drawCircle(SIGNAL_ICON_X, SIGNAL_ICON_Y, radius, ICON_SIGNAL_COLOR);
    delay(30);
  }

  for (int radius = ICON_OUTER_RADIUS; radius >= ICON_INNER_RADIUS; radius -= ICON_RING_SPACING) {
    
    tft.drawCircle(SIGNAL_ICON_X, SIGNAL_ICON_Y, radius, INVERTED_BG_COLOR);
    delay(30);
  }
}

void drawEmitSignal() {  // Draw circles for outgoing signal

  for (int radius = ICON_INNER_RADIUS; radius <= ICON_OUTER_RADIUS; radius += ICON_RING_SPACING){
    
    tft.drawCircle(SIGNAL_ICON_X, SIGNAL_ICON_Y, radius, ICON_SIGNAL_COLOR);
    delay(30);
  }

  for (int radius = ICON_INNER_RADIUS; radius <= ICON_OUTER_RADIUS; radius += ICON_RING_SPACING) {
    tft.drawCircle(SIGNAL_ICON_X, SIGNAL_ICON_Y, radius, DEFAULT_BG_COLOR);
    delay(30);
  }
}

void drawRemote(){
  if (receiveMode) {
    tft.fillScreen(INVERTED_BG_COLOR);
	  tft.setTextColor(INVERTED_TEXT_COLOR);
    tft.setTextSize(TEXT_SIZE_M);
	  tft.drawString(F("Select a button"), CENTER_X, CENTER_Y);
  } else {
    // Screen background
    tft.fillScreen(DEFAULT_BG_COLOR);

    // Middle button 
    tft.drawCircle(CENTER_X, CENTER_Y, CIRCLE_RADIUS + 2, OUTER_CIRCLE_COLOR);
    tft.fillCircle(CENTER_X, CENTER_Y, CIRCLE_RADIUS, CIRCLE_COLOR);
    tft.setTextSize(TEXT_SIZE_L);
    tft.setTextColor(DEFAULT_TEXT_COLOR);;
    tft.drawString(F("OK"), CENTER_X, CENTER_Y);

    // Draw top arrow
    tft.drawLine(CENTER_X, CENTER_Y - ARROW_TOP_OFFSET, CENTER_X + ARROW_LENGTH, CENTER_Y - ARROW_BASE_OFFSET, ARROW_COLOR);
    tft.drawLine(CENTER_X, CENTER_Y - ARROW_TOP_OFFSET, CENTER_X - ARROW_LENGTH, CENTER_Y - ARROW_BASE_OFFSET, ARROW_COLOR);

    // Draw right arrow
    tft.drawLine(CENTER_X + ARROW_TOP_OFFSET, CENTER_Y, CENTER_X + ARROW_BASE_OFFSET, CENTER_Y + ARROW_LENGTH, ARROW_COLOR);
    tft.drawLine(CENTER_X + ARROW_TOP_OFFSET, CENTER_Y, CENTER_X + ARROW_BASE_OFFSET, CENTER_Y - ARROW_LENGTH, ARROW_COLOR);

    // Draw bottom arrow
    tft.drawLine(CENTER_X, CENTER_Y + ARROW_TOP_OFFSET, CENTER_X - ARROW_LENGTH, CENTER_Y + ARROW_BASE_OFFSET, ARROW_COLOR);
    tft.drawLine(CENTER_X, CENTER_Y + ARROW_TOP_OFFSET, CENTER_X + ARROW_LENGTH, CENTER_Y + ARROW_BASE_OFFSET, ARROW_COLOR);

    // Draw left arrow
    tft.drawLine(CENTER_X - ARROW_TOP_OFFSET, CENTER_Y, CENTER_X - ARROW_BASE_OFFSET, CENTER_Y - ARROW_LENGTH, ARROW_COLOR);
    tft.drawLine(CENTER_X - ARROW_TOP_OFFSET, CENTER_Y, CENTER_X - ARROW_BASE_OFFSET, CENTER_Y + ARROW_LENGTH, ARROW_COLOR);

    // Draw power icon
    tft.drawRect(5, 9, 21, 2, ARROW_COLOR);
    tft.drawCircle(15, 22, 6, ARROW_COLOR);
    tft.drawLine(15, 16, 15, 20, ARROW_COLOR);

    // Draw wifi connection status icon
    drawWiFiConnectionIcon();

    // Draw bluetooth connection status icon
    drawBltConnectionIcon();
    
  }
}

void decideWiFiConnectionIcon(){
  // decide the color according to connection status and previous status so it doesn't loop
  if(receiveMode) return;
  if(wifiDeviceConnected == CONNECTED){
    if(wifiConnectedPrevVal == true){
      return; // if the wifi connection status is the same as before - do nothing
    }
    drawWiFiConnectionIcon(); // if different - draw the icon
  }else{
    if(wifiConnectedPrevVal == false){
      return; // if the wifi connection status is the same as before - do nothing
    }
    drawWiFiConnectionIcon(); // if different - draw the icon
  }
}

void drawWiFiConnectionIcon(){
  // decide the color according to connection status
  uint32_t color;
  if(wifiDeviceConnected == CONNECTED){
    color = WIFI_CONNECTION_ICON_COLOR_ON;
  }else{
    color = WIFI_CONNECTION_ICON_COLOR_OFF;
  }
  // empty the region for new icon
  tft.drawRect(WIFI_CONNECTION_CIRCLE_X - WIFI_CONNECTION_CIRCLE_MAX_RAD, WIFI_CONNECTION_CIRCLE_Y - WIFI_CONNECTION_CIRCLE_MAX_RAD, WIFI_CONNECTION_CIRCLE_MAX_RAD * 2, WIFI_CONNECTION_CIRCLE_MAX_RAD * 2, DEFAULT_BG_COLOR);

  // draw 3 circles
  for(int i = 0; i < 3; i++){
    tft.drawCircle(WIFI_CONNECTION_CIRCLE_X, WIFI_CONNECTION_CIRCLE_Y, WIFI_CONNECTION_CIRCLE_MAX_RAD - WIFI_CONNECTION_CIRCLE_RADIUS_DIFF * i, color);
  }

  // use 2 triangles to mask the 3 circles into 3 arcs
  tft.fillTriangle(WIFI_CONNECTION_CIRCLE_X - WIFI_CONNECTION_CIRCLE_MAX_RAD, WIFI_CONNECTION_CIRCLE_Y + WIFI_CONNECTION_CIRCLE_MAX_RAD, WIFI_CONNECTION_CIRCLE_X + WIFI_CONNECTION_CIRCLE_MAX_RAD, WIFI_CONNECTION_CIRCLE_Y - WIFI_CONNECTION_CIRCLE_MAX_RAD, WIFI_CONNECTION_CIRCLE_X + WIFI_CONNECTION_CIRCLE_MAX_RAD, WIFI_CONNECTION_CIRCLE_Y + WIFI_CONNECTION_CIRCLE_MAX_RAD, DEFAULT_BG_COLOR);
  tft.fillTriangle(WIFI_CONNECTION_CIRCLE_X + WIFI_CONNECTION_CIRCLE_MAX_RAD, WIFI_CONNECTION_CIRCLE_Y + WIFI_CONNECTION_CIRCLE_MAX_RAD, WIFI_CONNECTION_CIRCLE_X - WIFI_CONNECTION_CIRCLE_MAX_RAD, WIFI_CONNECTION_CIRCLE_Y - WIFI_CONNECTION_CIRCLE_MAX_RAD, WIFI_CONNECTION_CIRCLE_X - WIFI_CONNECTION_CIRCLE_MAX_RAD, WIFI_CONNECTION_CIRCLE_Y + WIFI_CONNECTION_CIRCLE_MAX_RAD, DEFAULT_BG_COLOR);
}

void decideBltConnectionIcon(){
	// decide the color according to connection status and previous status so it doesn't loop
  if(receiveMode) return;
  if(bleDeviceConnected){
    if(bltConnectedPrevVal == true){
      return; // if the bluetooth connection status is the same as before - do nothing
    }
    drawBltConnectionIcon(); // if different - draw the icon
  }else{
    if(bltConnectedPrevVal == false){
      return; // if the bluetooth connection status is the same as before - do nothing
    }
    drawBltConnectionIcon(); // if different - draw the icon
  }
}


void drawBltConnectionIcon(){
  // decide the color according to connection status
  uint32_t color;
  if(bleDeviceConnected){
    color = BLT_ICON_COLOR_ON;
  }else{
    color = BLT_ICON_COLOR_OFF;
  }

  // empty the region for new icon
  tft.drawRect(BLT_ICON_START_X, BLT_ICON_START_Y, BLT_ICON_WIDTH, BLT_ICON_HEIGHT, DEFAULT_BG_COLOR);

  // draw the bluetooth icon
  tft.drawLine(BLT_ICON_START_X, BLT_ICON_START_Y + BLT_ICON_HEIGHT * 3/4, BLT_ICON_START_X + BLT_ICON_WIDTH, BLT_ICON_START_Y + BLT_ICON_HEIGHT * 1/4, color);
  tft.drawLine(BLT_ICON_START_X + BLT_ICON_WIDTH, BLT_ICON_START_Y + BLT_ICON_HEIGHT * 1/4, BLT_ICON_START_X + BLT_ICON_WIDTH * 1/2, BLT_ICON_START_Y, color);
  tft.drawLine(BLT_ICON_START_X + BLT_ICON_WIDTH * 1/2, BLT_ICON_START_Y, BLT_ICON_START_X + BLT_ICON_WIDTH * 1/2, BLT_ICON_START_Y + BLT_ICON_HEIGHT, color);
  tft.drawLine(BLT_ICON_START_X + BLT_ICON_WIDTH * 1/2, BLT_ICON_START_Y + BLT_ICON_HEIGHT, BLT_ICON_START_X + BLT_ICON_WIDTH, BLT_ICON_START_Y + BLT_ICON_HEIGHT * 3/4, color);
  tft.drawLine(BLT_ICON_START_X + BLT_ICON_WIDTH, BLT_ICON_START_Y + BLT_ICON_HEIGHT * 3/4, BLT_ICON_START_X, BLT_ICON_START_Y + BLT_ICON_HEIGHT * 1/4, color);
}

void emitData(const Command& command){
	if (command.rawData != nullptr){
		emitter.send(command.rawData, command.dataLength, CARRIER_FREQUENCY_KHZ);
    drawEmitSignal();

    #ifdef DEBUG_LOG
      Serial.print(F("Signal sent: [")); Serial.print(command.dataLength); Serial.print(F("]{"));

      for (uint8_t i = 0; i < command.dataLength; i++) {
        Serial.print(command.rawData[i]);

        if (i != command.dataLength - 1) {
          Serial.print(F(", "));
        }
      }

      Serial.println(F("}"));
    #endif
	}
}

void switchMode(){
  if (receiveMode) {
    chosenButton = -1; // Forget the chosen button when the user exits cloning mode
    mappingToCustomButton = false;
    receiver.disableIRIn(); // Disable IR input when exiting cloning mode to prevent reading random signals
  } 
  receiveMode = !receiveMode;

  mqttPublishWithLog(TOPIC_CURRENT_MODE, receiveMode ? "CLONE" : "EMIT");
  
  startBuzzer();
	drawRemote();
}

bool canMapButtons() {
	for (uint8_t i = 0; i < BTN_COUNT; i++) {
    if (commandMap[i].dataLength == 0) {
      return true;
    }
  }

	return false;
}

void receive() {
	if (!canMapButtons()) {
		switchMode();
		return;
	}

  int pressedButton = getButtonPressedIndex();
  if ((pressedButton != -1 || chosenFromApp) && (pressedButton != chosenButton || mappingToCustomButton)) {
    if(!chosenFromApp) chosenButton = pressedButton;
    tft.fillRect(0, CENTER_Y - tft.fontHeight(TEXT_SIZE_M)/2, TFT_HEIGHT, tft.fontHeight(TEXT_SIZE_M), TFT_WHITE);
    
    tft.setTextSize(TEXT_SIZE_M);
    char message[16];
    sprintf(message, "%s selected!", mappingToCustomButton ? "CUSTOM" : getButtonName(chosenButton));
    tft.drawString(message, CENTER_X, CENTER_Y);
    
    tft.setTextSize(TEXT_SIZE_S);
    tft.drawString(F("Waiting for IR."), CENTER_X, CENTER_Y + 20);
    
    // Reset logic values to not enter this block every loop
    chosenFromApp = false;
    pressedButton = -1;
  }
  
  // wait until a button is pressed
  if (chosenButton != -1) {
    receiver.enableIRIn(); // Enable receiving only after a button is pressed
    // NOTE: IR input is automatically disabled after a signal is received

	  if (receiver.getResults()){
		  uint8_t dataLength = recvGlobal.recvLength;
		  uint16_t *rawData = new uint16_t[dataLength];

      for (uint8_t i = 1; i < dataLength; i++) {
        rawData[i - 1] = recvGlobal.recvBuffer[i];
      }

      rawData[dataLength - 1] = 1000; // Arbitrary trailing space

      Command recCommand;
      if (!mappingToCustomButton) {
        recCommand = {rawData, dataLength, -1 * (chosenButton + 1)};
        commandMap[chosenButton] = recCommand; // Write the received command to the map
      }
      else {
        recCommand = {rawData, dataLength, chosenButton};
      }
      
      mqttPublishWithLog(TOPIC_IR_OUT, serializeCommand(recCommand));

      tft.drawString(F("Recorded!"), CENTER_X, CENTER_Y + 40);
      drawReceiveSignal();

      drawRemote(); // Reset the UI

      // Reset logic variables
      if (mappingToCustomButton) switchMode();
      mappingToCustomButton = false;
      chosenButton = -1;
    }
  } 
}

void setup() {
  Serial.begin(9600); // Start serial
  while(!Serial); // Wait for serial

  setupWiFi();
  setupMQTT();
  setupBLE();
  
  // Set up pins
	pinMode(PRESS_BTN, INPUT_PULLUP);
	pinMode(UP_BTN, INPUT_PULLUP);
  pinMode(DOWN_BTN, INPUT_PULLUP);
  pinMode(LEFT_BTN, INPUT_PULLUP);
  pinMode(RIGHT_BTN, INPUT_PULLUP);
	pinMode(WIO_KEY_A, INPUT_PULLUP);
	pinMode(WIO_KEY_B, INPUT_PULLUP);
	pinMode(WIO_KEY_C, INPUT_PULLUP);

	pinMode(MO_PIN, OUTPUT); 
  
  pinMode(BUZZER_PIN, OUTPUT);

  // Initialize commands "map"
  for(uint8_t i = 0; i < BTN_COUNT; i++){
    commandMap[i].rawData = new uint16_t[0];
	  commandMap[i].dataLength = 0;
  }

  // Screen setup
  tft.begin();
  tft.setRotation(SCREEN_ROTATION);
  tft.setTextDatum(MC_DATUM);

	drawRemote();
}

void loop() {
  updateNetwork();
  updateVibration();
  updateBuzzer();

  // Mode button logic
  bool modeBtnState = digitalRead(MODE_BTN);
  if (modeBtnState != prevModeBtnState) {
    if (modeBtnState == HIGH){
		  switchMode();
    }
  }

  prevModeBtnState = modeBtnState;

	if (receiveMode){
		receive();
	} else { // Detecting button presses
    int pressed = getButtonPressedIndex();

    if (pressed != -1) {
      const Command command = commandMap[pressed];

			if (command.dataLength != 0){
        emitData(command);

        startVibration(); // Vibrate after data sent

        #ifdef DEBUG_UI // Flash a circle next to the pressed button label
          switch(pressed) {
            case POWER_BTN:
              tft.fillCircle(0, 16, 4, TFT_GREEN);
              break;
            case UP_BTN:
              tft.fillCircle(5, 36, 4, TFT_GREEN);
              break;
            case LEFT_BTN:
              tft.fillCircle(5, 56, 4, TFT_GREEN);
              break;
            case RIGHT_BTN:
              tft.fillCircle(5, 76, 4, TFT_GREEN);
              break;
            case DOWN_BTN:
              tft.fillCircle(5, 96, 4, TFT_GREEN);
              break;
            case PRESS_BTN:
              tft.fillCircle(5, 116, 4, TFT_GREEN);
              break;
          }
        #endif
        

        #ifdef DEBUG_UI
          tft.fillRect(0, 0, 10, 124, TFT_BLACK); // Erase the circle
        #endif
      }
    }
  }

  // DEBUG: print configured buttons on screen
  #ifdef DEBUG_UI
    tft.setTextSize(TEXT_SIZE_M);

    if (commandMap[POWER_BTN_INDEX].dataLength != 0) {
      tft.drawString(F("POWER"), 20, 20);
    } else if (commandMap[UP_BTN_INDEX].dataLength != 0) {
      tft.drawString(F("UP"), 20, 40);
    } else if (commandMap[LEFT_BTN_INDEX].dataLength != 0) {
      tft.drawString(F("LEFT"), 20, 60);
    } else if (commandMap[RIGHT_BTN_INDEX].dataLength != 0) {
      tft.drawString(F("RIGHT"), 20, 80);
    } else if (commandMap[DOWN_BTN_INDEX].dataLength != 0) {
      tft.drawString(F("DOWN"), 20, 100);
    } else if (commandMap[PRESS_BTN_INDEX].dataLength != 0) {
      tft.drawString(F("OK"), 20, 120);
    }
  #endif
  delay(50); // Slow down the loop
}