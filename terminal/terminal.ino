// Developer modes
//#define DEBUG_LOG             // log events to serial
//#define DEBUG_CONFIG_CREATOR  // unlocks config creation mode (key B) 

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
#include "Button.h"
#include "UIManager.h"


// Button indexes for the array acting as a map
#define BTN_COUNT       6
#define POWER_BTN_INDEX 0 // -1 in the app
#define UP_BTN_INDEX    1 // -2 in the app
#define RIGHT_BTN_INDEX 2 // -3 in the app
#define DOWN_BTN_INDEX  3 // -4 in the app
#define LEFT_BTN_INDEX  4 // -5 in the app
#define OK_BTN_INDEX 5    // -6 in the app

// Motor pin
#define MO_PIN D0

// Buzzer pin
#define BUZZER_PIN WIO_BUZZER 

// Buzzer constants
#define BUZZER_FRQ 128 // Buzzer PWM frequency

// Bluetooth
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

// WiFi
#define CONNECTED    0
#define CONNECTING   1
#define DISCONNECTED 2

// IR
#define CARRIER_FREQUENCY_KHZ 38

// Define a common logger for all classes if in debug mode
#if defined(DEBUG_LOG) || defined(DEBUG_CONFIG_CREATOR) 
Logger* logger = new Logger(); 
#endif

// Buttons
Button upBtn(WIO_5S_UP, "UP");
Button downBtn(WIO_5S_DOWN, "DOWN");
Button leftBtn(WIO_5S_LEFT, "LEFT");
Button rightBtn(WIO_5S_RIGHT, "RIGHT");
Button okBtn(WIO_5S_PRESS, "OK");
Button powerBtn(WIO_KEY_C, "POWER");
Button modeBtn(WIO_KEY_A, "MODE");
// Devmode buttons
Button configRecBtn(WIO_KEY_B, "REC");
Button configSkipBtn(WIO_KEY_C, "SKIP");

// Button map to commands
Command **commandMap = new Command*[BTN_COUNT];

// UI
UIManager ui;

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
TerminalMode mode = TerminalMode::EMIT;
int chosenButton = -1; // Button selection in the cloning mode
bool chosenFromApp = false;
bool mappingToCustomButton = false;

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

class BluetoothServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* bleServer) {
      #ifdef DEBUG_LOG
        logger->log(F("Bluetooth connected\n"));
      #endif

      bleDeviceConnected = true;
      ui.updateBltIconStatus(true, mode);
    }

    void onDisconnect(BLEServer* bleServer) {
      #ifdef DEBUG_LOG
        logger->log(F("Bluetooth disconnected\n"));
      #endif

      bleDeviceConnected = false;
      ui.updateBltIconStatus(false, mode);
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

  // A command sent from the app
  if (strcmp(topic, TOPIC_IR_IN) == 0) {
    Command* command = new Command(buff_p);

    if (mode == TerminalMode::EMIT) { // if in emit mode, emit the signal
      emitData(command);
    } else { // if in cloning mode, register the received button for cloning
      chosenButton = -1 * (command->getKeyCode() + 1);
      chosenFromApp = true;
    }
    delete command; // free the memory used for received command
  }
  // Request to change the mode from the app
  else if (strcmp(topic, TOPIC_SWITCH_MODE) == 0) {
    if (strstr(buff_p, "CLONE") != NULL) { // cloning mode requested (Message format: CLONE<keyCode>)
      if (mode != TerminalMode::CLONE) {
        changeMode(TerminalMode::CLONE);
      } 
      chosenButton = atoi(buff_p + 5); // skip 5 characters ("CLONE") to get the keyCode
      chosenFromApp = true;
      mappingToCustomButton = true;
    }
    else if (strcmp(buff_p, "EMIT") == 0) { // emit mode requested
      if (mode != TerminalMode::EMIT) {
        changeMode(TerminalMode::EMIT);
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

      WiFi.begin(ssid, wifiInfo["password"], 0L, stringToMAC(wifiInfo["bssid"]));
    }
  }

  ui.updateWiFiIconStatus(wifiDeviceConnected == CONNECTED, mode);
}

int getButtonPressedIndex(){
  int out = -1;

  if (upBtn.isPressed()) {
    out = UP_BTN_INDEX;
  } else if (downBtn.isPressed()) {
    out = DOWN_BTN_INDEX;
  } else if (leftBtn.isPressed()) {
    out = LEFT_BTN_INDEX;
  } else if (rightBtn.isPressed()) {
    out = RIGHT_BTN_INDEX;
  } else if (okBtn.isPressed()) {
    out = OK_BTN_INDEX;
  } else if (powerBtn.isPressed()) {
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
    case OK_BTN_INDEX:
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

void emitData(Command *command){
  if (command != nullptr){
	  if(command->getDataLength() == 0) return; // command is empty, do nothing
      emitter.send(command->getRawData(), command->getDataLength(), CARRIER_FREQUENCY_KHZ);
      ui.playEmitSignalAnimation();

      #ifdef DEBUG_LOG
        logger->logIR(command);
      #endif
  }
}

void switchMode() {
  if (mode == TerminalMode::CLONE) 
    changeMode(TerminalMode::EMIT);
  else if (mode == TerminalMode::EMIT)
    changeMode(TerminalMode::CLONE);
}

void changeMode(TerminalMode newMode){
  if (mode == TerminalMode::CLONE) {
    chosenButton = -1; // Forget the chosen button when the user exits cloning mode
    mappingToCustomButton = false;
    receiver.disableIRIn(); // Disable IR input when exiting cloning mode to prevent reading random signals
  } 

  mode = newMode;
  startBuzzer();
  ui.redraw(mode);

  if (mode == TerminalMode::CLONE)
    mqttClient->publishWithLog(TOPIC_CURRENT_MODE, "CLONE");
  else if (mode == TerminalMode::EMIT)
    mqttClient->publishWithLog(TOPIC_CURRENT_MODE, "EMIT");
  
  if (mode != TerminalMode::CLONE){
    receiver.disableIRIn();
  }
}

#ifdef DEBUG_CONFIG_CREATOR
void switchConfigMode(){ // Switches between config mode and normal mode
  if (mode == TerminalMode::CONFIG) {
    changeMode(TerminalMode::EMIT);
  }
  else {
    for(int i = 0; i < configTextsLength; i++){ // Clear the recorded commands
      configCommandsList[i].rawData = new uint16_t[0];
      configCommandsList[i].dataLength = 0;
    }
    drawConfigDebug();
  }
}
#endif

void receive() {
  int pressedButton = getButtonPressedIndex();
  if ((pressedButton != -1 || chosenFromApp) && (pressedButton != chosenButton || mappingToCustomButton)) {
    if(!chosenFromApp) chosenButton = pressedButton;

    ui.drawButtonSelected(mappingToCustomButton ? "CUSTOM" : getButtonName(chosenButton));
    
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

      ui.playReceiveSignalAnimation();

      ui.redraw(mode); // Reset the UI

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
  pinMode(MO_PIN, OUTPUT);   
  pinMode(BUZZER_PIN, OUTPUT);

  // Screen setup
  ui.setup();
  ui.redraw(mode);
}

void loop() {
  updateNetwork();
  updateVibration();
  updateBuzzer();

  if (modeBtn.isPressed() && mode != TerminalMode::CONFIG) {
    switchMode();
  }

  #ifdef DEBUG_CONFIG_CREATOR
  if (configBtn.isPressed() && mode != TerminalMode::CLONE) {
    switchConfigMode();
    completedConfigsCount = 0;
  }
  #endif

  switch (mode) {
    case TerminalMode::CLONE:
      receive();
      break;
    case TerminalMode::CONFIG:
      #ifdef DEBUG_CONFIG 
      receiveConfig();
      #endif
      break;
    case TerminalMode::EMIT:
      int pressed = getButtonPressedIndex();
  
      if (pressed != -1) {
        Command *command = commandMap[pressed];

        if (command != nullptr) {
         emitData(command);

         startVibration(); // Vibrate after data sent
        }
      }
      break;
  }
}