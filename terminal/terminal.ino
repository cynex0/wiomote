// Debugging modes
//#define DEBUG_UI    // additional UI elements
//#define DEBUG_LOG   // log events to serial
//#define DEBUG_CONFIG_CREATOR // allows to quickly create a config with a middle button (key B) 

#include <ArduinoJson.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include <rpcWiFi.h>

#include <IRLibSendBase.h>
#include <IRLib_HashRaw.h>
#include <IRLibCombo.h>
#include <IRLibRecvPCI.h>

#include <TFT_eSPI.h>

#include "WioMqttClient.h" 
#include "Command.h"
#include "Logger.h"


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
#define UP_BTN        WIO_5S_UP
#define DOWN_BTN    WIO_5S_DOWN
#define LEFT_BTN    WIO_5S_LEFT
#define RIGHT_BTN  WIO_5S_RIGHT
#define PRESS_BTN  WIO_5S_PRESS
#define POWER_BTN     WIO_KEY_C
#define MODE_BTN      WIO_KEY_A
#define CONFIG_REC_BTN  WIO_KEY_B
#define CONFIG_SKIP_BTN WIO_KEY_C

// Bluetooth
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

// WiFi
#define CONNECTED    0
#define CONNECTING   1
#define DISCONNECTED 2

// IR
#define CARRIER_FREQUENCY_KHZ 38

// Wifi connection icon
#define WIFI_ICON_CIRCLE_X                290
#define WIFI_ICON_CIRCLE_Y                 35
#define WIFI_ICON_CIRCLE_MAX_RAD           21
#define WIFI_ICON_CIRCLE_RADIUS_DIFF        7
#define WIFI_ICON_ICON_COLOR_ON      TFT_CYAN
#define WIFI_ICON_ICON_COLOR_OFF TFT_DARKGREY

// Bluetooth connection icon
#define BLT_ICON_START_X            245
#define BLT_ICON_START_Y             11
#define BLT_ICON_WIDTH               15
#define BLT_ICON_HEIGHT              25
#define BLT_ICON_COLOR_ON      TFT_CYAN
#define BLT_ICON_COLOR_OFF TFT_DARKGREY

// Define a common logger for all classes if in debug mode
#if defined(DEBUG_LOG) || defined(DEBUG_CONFIG_CREATOR) 
Logger* logger = new Logger(); 
#endif

// Button map to commands
Command **commandMap = new Command*[BTN_COUNT];

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

// Pointer to WioMqttClient instance. Initialized in setup()
WioMqttClient *mqttClient;

// Logic variables
bool receiveMode = false;
bool configMode = false;
bool prevModeBtnState = HIGH;
int chosenButton = -1; // Button selection in the cloning mode
bool chosenFromApp = false;
bool mappingToCustomButton = false;
bool prevConfigBtnState = HIGH;
bool prevConfigSkipBtnState = HIGH;
bool wifiConnectedPrevVal = true;
bool bltConnectedPrevVal = false;

// Motor variables
const int vibDuration = 200;
unsigned long lastVibrated = 0;
bool isVibrating = false;

// Buzzer variables
const int buzzDuration = 400;
unsigned long lastBuzzed = 0;
bool isBuzzing = false;

#ifdef DEBUG_CONFIG_CREATOR
int completedConfigsCount = 0;
const int configTextsLength = 11; // number of config buttons that will be recorded (MAX 11)
char** configTexts = new char*[configTextsLength]{ // label name for each config command
  "POWER",
  "UP",
  "RIGHT",
  "DOWN",
  "LEFT",
  "PRESS",
  "CHANNEL UP",
  "CHANNEL DOWN",
  "VOLUME UP",
  "VOLUME DOWN",
  "MUTE"
};
Command **configCommandsList = new Command*[configTextsLength]; // array to store recorded commands that will be later serialized into one config
#endif

void decideBltConnectionIcon(){
  // decide the color according to connection status and previous status so it doesn't loop
  if(receiveMode || configMode) return;
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

class BluetoothServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* bleServer) {
      #ifdef DEBUG_LOG
        logger->log(F("Bluetooth connected\n"));
      #endif

      bleDeviceConnected = true;
      decideBltConnectionIcon();
      bltConnectedPrevVal = true;
    }

    void onDisconnect(BLEServer* bleServer) {
      #ifdef DEBUG_LOG
        logger->log(F("Bluetooth disconnected\n"));
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

      WiFi.disconnect();

      wifiDeviceConnected = DISCONNECTED;

      #ifdef DEBUG_LOG
        logger->log(F(data));
        logger->log("\n");
        logger->log(F("Cleared existing connections\n"));
      #endif
    }
};

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char* buff_p = new char[length];
  for (int i = 0; i < length; i++) {
    buff_p[i] = (char) payload[i];
  }
  buff_p[length] = '\0';


  #ifdef DEBUG_LOG
    logger->logMqtt(topic, buff_p, MqttMessageDirection::IN);
  #endif
  #ifdef DEBUG_UI
    drawRemote();
    tft.setTextSize(TEXT_SIZE_S);
    tft.setCursor(0, TFT_WIDTH - 2); // width is height :)
    tft.print(F("MQTT: ")); tft.print(F(buff_p));
  #endif

  // A command sent from the app
  if (strcmp(topic, TOPIC_IR_IN) == 0) {
    Command* command = new Command(buff_p);

    if (!receiveMode) { // if in receive mode, emit the signal
      emitData(command);
    } else { // if in cloning mode, register the received button for cloning
      chosenButton = -1 * (command->getKeyCode() + 1);
      chosenFromApp = true;
    }
    delete command; // free the memory used for received command
  }
  else if (strcmp(topic, TOPIC_SWITCH_MODE) == 0) {
    if (strstr(buff_p, "CLONE") != NULL) { // cloning mode requested (Message format: CLONE<keyCode>)
      if (!receiveMode) {
        switchMode();
      } 
      chosenButton = atoi(buff_p + 5); // skip 5 characters ("CLONE") to get the keyCode
      chosenFromApp = true;
      mappingToCustomButton = true;
    }
    else if (strcmp(buff_p, "EMIT") == 0) { // emit mode requested
      if (receiveMode) {
        switchMode();
      }
    }
  }

  delete[] buff_p;
}

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
        logger->logWifiConnected(WiFi.SSID(), WiFi.localIP().toString());
      }
    #endif

    wifiDeviceConnected = CONNECTED;

    decideWiFiConnectionIcon();
    wifiConnectedPrevVal = true;

    mqttClient->update();
  } 
  else {
    const char *ssid = wifiInfo["ssid"];

    if(wifiDeviceConnected != CONNECTING) {  
      #ifdef DEBUG_LOG
        logger->log(F("Connecting to ")); 
        logger->log(ssid);
        logger->log("\n");
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


int convertIndexToKeyCode(const int index) { // Converts the index of the button to the app's key code
  if(index > BTN_COUNT - 1) return index - BTN_COUNT;
  return -1 * (index + 1);
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
  if(receiveMode || configMode) return;
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
  // empty the region for new icon
  tft.drawRect(WIFI_ICON_CIRCLE_X - WIFI_ICON_CIRCLE_MAX_RAD, WIFI_ICON_CIRCLE_Y - WIFI_ICON_CIRCLE_MAX_RAD, WIFI_ICON_CIRCLE_MAX_RAD * 2, WIFI_ICON_CIRCLE_MAX_RAD * 2, DEFAULT_BG_COLOR);

  // draw 3 circles
  for(int i = 0; i < 3; i++){
    tft.drawCircle(WIFI_ICON_CIRCLE_X, WIFI_ICON_CIRCLE_Y, WIFI_ICON_CIRCLE_MAX_RAD - WIFI_ICON_CIRCLE_RADIUS_DIFF * i, wifiDeviceConnected == CONNECTED ? WIFI_ICON_ICON_COLOR_ON : WIFI_ICON_ICON_COLOR_OFF);
  }

  // use 2 triangles to mask the 3 circles into 3 arcs
  tft.fillTriangle(WIFI_ICON_CIRCLE_X - WIFI_ICON_CIRCLE_MAX_RAD, WIFI_ICON_CIRCLE_Y + WIFI_ICON_CIRCLE_MAX_RAD, WIFI_ICON_CIRCLE_X + WIFI_ICON_CIRCLE_MAX_RAD, WIFI_ICON_CIRCLE_Y - WIFI_ICON_CIRCLE_MAX_RAD, WIFI_ICON_CIRCLE_X + WIFI_ICON_CIRCLE_MAX_RAD, WIFI_ICON_CIRCLE_Y + WIFI_ICON_CIRCLE_MAX_RAD, DEFAULT_BG_COLOR);
  tft.fillTriangle(WIFI_ICON_CIRCLE_X + WIFI_ICON_CIRCLE_MAX_RAD, WIFI_ICON_CIRCLE_Y + WIFI_ICON_CIRCLE_MAX_RAD, WIFI_ICON_CIRCLE_X - WIFI_ICON_CIRCLE_MAX_RAD, WIFI_ICON_CIRCLE_Y - WIFI_ICON_CIRCLE_MAX_RAD, WIFI_ICON_CIRCLE_X - WIFI_ICON_CIRCLE_MAX_RAD, WIFI_ICON_CIRCLE_Y + WIFI_ICON_CIRCLE_MAX_RAD, DEFAULT_BG_COLOR);
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

void emitData(Command *command){
  if (command != nullptr){
	  if(command->getDataLength() == 0) return; // command is empty, do nothing
      emitter.send(command->getRawData(), command->getDataLength(), CARRIER_FREQUENCY_KHZ);
      drawEmitSignal();

      #ifdef DEBUG_LOG
        logger->logIR(command);
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

  mqttClient->publishWithLog(TOPIC_CURRENT_MODE, receiveMode ? "CLONE" : "EMIT");
  
  if(!receiveMode){
    receiver.disableIRIn();
  }
  startBuzzer();
  drawRemote();
}

#ifdef DEBUG_CONFIG_CREATOR
void switchConfigMode(){ // Switches between config mode and normal mode
  configMode = !configMode;
  if(configMode){ // If in config mode
    for(int i = 0; i < configTextsLength; i++){ // Clear the recorded commands
      configCommandsList[i].rawData = new uint16_t[0];
      configCommandsList[i].dataLength = 0;
    }
      drawConfigDebug();
  }else{
    receiver.disableIRIn();
    drawRemote();
  }
  startBuzzer();
}
#endif

void receive() {
  int pressedButton = getButtonPressedIndex();
  if ((pressedButton != -1 || chosenFromApp) && (pressedButton != chosenButton || mappingToCustomButton)) {
    if(!chosenFromApp) chosenButton = pressedButton;
    tft.fillRect(0, CENTER_Y - tft.fontHeight(TEXT_SIZE_M)/2, TFT_HEIGHT, tft.fontHeight(TEXT_SIZE_M), TFT_WHITE);
    
    tft.setTextSize(TEXT_SIZE_M);
    char message[16]; // Max length: 16 = 10 (" selected!") + 6 ("CUSTOM")
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

      Command* command;
      if (!mappingToCustomButton) { // If mapping to a physycal button, save command on the terminal
        command = new Command(rawData, dataLength, convertIndexToKeyCode(chosenButton));
        commandMap[chosenButton] = command; // Write the received command to the map
      } else {
        // if recording a custom button, chosenButton is sent from the app and is already a keyCode
        command = new Command(rawData, dataLength, chosenButton); 
      }

      const char* jsonCommand = command->serialize(getButtonName(-1 * (command->getKeyCode() + 1)));
      mqttClient->publishWithLog(TOPIC_IR_OUT, jsonCommand);
      delete[] jsonCommand;

      tft.drawString(F("Recorded!"), CENTER_X, CENTER_Y + 40);
      drawReceiveSignal();

      drawRemote(); // Reset the UI

      if (mappingToCustomButton) {
        switchMode(); // don't continue cloning if mapping to a custom button
        delete command; // the terminal does not store commands for custom buttons, memory can be safely freed
      }

      // Reset logic variables
      mappingToCustomButton = false;
      chosenButton = -1;
    }
  } 
}

#ifdef DEBUG_CONFIG_CREATOR
void drawConfigDebug(){
  tft.fillScreen(INVERTED_BG_COLOR);
  tft.setTextColor(INVERTED_TEXT_COLOR);
  tft.setTextSize(TEXT_SIZE_M);
  for(int i = 0; i < configTextsLength; i++){
    tft.drawString(configTexts[i], 20, 20 + 20 * i); // Draw the labels for each config button
  }
}

void receiveConfig(){
  receiver.enableIRIn();
  
  // logic for skipping a command
  if(digitalRead(CONFIG_SKIP_BTN) != prevConfigSkipBtnState){
	  if(digitalRead(CONFIG_SKIP_BTN) == LOW){
      tft.setTextColor(TFT_RED);
      tft.drawString(F("SKIPPED"), 200, 20 + 20 * completedConfigsCount); // Draw "RECORDED" next to the labels
      const uint8_t dataLength = 0;
      uint16_t *rawData = new uint16_t[dataLength];
      completedConfigsCount++;
    }
  }
  prevConfigSkipBtnState = digitalRead(CONFIG_SKIP_BTN);
  
  if (receiver.getResults()){ // If a signal is received
    const uint8_t dataLength = recvGlobal.recvLength;
    uint16_t *rawData = new uint16_t[dataLength];
  
    for (uint8_t i = 1; i < dataLength; i++) {
      rawData[i - 1] = recvGlobal.recvBuffer[i];
    }

    rawData[dataLength - 1] = 1000; // Arbitrary trailing space
    Command *recCommand = new Command(rawData, dataLength, completedConfigsCount);
    configCommandsList[completedConfigsCount] = recCommand; // Save the received command to the list

    tft.setTextColor(TFT_DARKGREEN);
    tft.drawString(F("RECORDED"), 200, 20 + 20 * completedConfigsCount); // Draw "RECORDED" next to the labels

    completedConfigsCount++;
    drawReceiveSignal();
  }

  if(completedConfigsCount == configTextsLength){ // If all the config buttons have been recorded
    delay(500);

    completedConfigsCount = 0;
	  int appsConfigBtnCount = BTN_COUNT; // Start from the first app button

    switchConfigMode();

    // Serialize the recorded commands into one config
    JsonDocument* doc = new JsonDocument;
    JsonArray commandsArray = doc->to<JsonArray>();
    for(int i = 0; i < configTextsLength; i++){ // Loop through the recorded commands
      if(configCommandsList[i]->getDataLength() == 0) continue; // Skip if no command was recorded (skipped button)
      if(i > BTN_COUNT - 1) { // If the button is an app button
        commandsArray.add(configCommandsList[i]->serializeToDoc(convertIndexToKeyCode(appsConfigBtnCount), configTexts[i], configCommandsList[i]));
        appsConfigBtnCount++;
      } else {
        commandsArray.add(serializeCommandToDoc(convertIndexToKeyCode(i), configTexts[i], configCommandsList[i]));
      }
    }
    String out;
    serializeJson(*doc, out);
    logger->log(doc);
  }
}
#endif

void setup() {
  #if defined(DEBUG_LOG) || defined(DEBUG_CONFIG_CREATOR)
  logger->begin();
  #endif

  // Create an MQTT client depending on if debug is enabled
  #ifdef DEBUG_LOG
  mqttClient = new WioMqttClient(wifiClient, *mqttCallback, logger);
  #else
  mqttClient = new WioMqttClient(wifiClient, *mqttCallback);
  #endif

  for (int i = 0; i < BTN_COUNT; i++)
    commandMap[i] = nullptr; // initialize array with null pointers to avoid garbage data

  setupWiFi();
  mqttClient->setup();
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
  if (modeBtnState != prevModeBtnState && !configMode) {
    if (modeBtnState == HIGH){
      switchMode();
    }
  }
  prevModeBtnState = modeBtnState;

  #ifdef DEBUG_CONFIG_CREATOR
  // Config button logic
  bool configBtnState = digitalRead(CONFIG_REC_BTN);
  if (configBtnState != prevConfigBtnState && !receiveMode) {
    if (configBtnState == HIGH){
      switchConfigMode();
      completedConfigsCount = 0;
    }
  }
  prevConfigBtnState = configBtnState;
  #endif

  if (receiveMode){
    receive();
  }
  #ifdef DEBUG_CONFIG_CREATOR
  else if (configMode){
    receiveConfig();
  } 
  #endif
  else { // Detecting button presses
    int pressed = getButtonPressedIndex();
  
    if (pressed != -1) {
      Command *command = commandMap[pressed];

      if (command != nullptr){
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

    if (commandMap[POWER_BTN_INDEX] != nullptr) {
      tft.drawString(F("POWER"), 20, 20);
    } else if (commandMap[UP_BTN_INDEX] != nullptr) {
      tft.drawString(F("UP"), 20, 40);
    } else if (commandMap[LEFT_BTN_INDEX] != nullptr) {
      tft.drawString(F("LEFT"), 20, 60);
    } else if (commandMap[RIGHT_BTN_INDEX] != nullptr) {
      tft.drawString(F("RIGHT"), 20, 80);
    } else if (commandMap[DOWN_BTN_INDEX] != nullptr) {
      tft.drawString(F("DOWN"), 20, 100);
    } else if (commandMap[PRESS_BTN_INDEX] != nullptr) {
      tft.drawString(F("OK"), 20, 120);
    }
  #endif
}