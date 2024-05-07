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

// Debugging mode
#define DEBUG

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

// UI elements
#define CIRCLE_COLOR        TFT_BLUE
#define OUTER_CIRCLE_COLOR TFT_WHITE
#define CIRCLE_RADIUS             45
#define CENTER_X                 160  // Middle point of screen X-axis
#define CENTER_Y                 120  // Middle point of screen Y-axis
#define SCREEN_ROTATION            3

#define ARROW_TOP_OFFSET  100  // Distance from middle to the top of the arrows
#define ARROW_BASE_OFFSET  60  // Distance from middle to bottom sides of arrows
#define ARROW_LENGTH       40  // Value of arrow length
#define ARROW_COLOR TFT_WHITE

#define TEXT_SIZE_L          3
#define TEXT_SIZE_M          2
#define TEXT_SIZE_S          1

#define BACKGROUND_COLOR TFT_BLACK  // Define screen color

// Buttons
#define UP_BTN        WIO_5S_UP
#define DOWN_BTN    WIO_5S_DOWN
#define LEFT_BTN    WIO_5S_LEFT
#define RIGHT_BTN  WIO_5S_RIGHT
#define PRESS_BTN  WIO_5S_PRESS
#define POWER_BTN     WIO_KEY_C
#define MODE_BTN      WIO_KEY_A

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

#define TOPIC_CONN_OUT "wiomote/connection/terminal"
#define TOPIC_CONN_IN       "wiomote/connection/app"
#define TOPIC_APP_COMMAND           "wiomote/ir/app"

// IR
#define CARRIER_FREQUENCY_KHZ 38

struct Command {
  uint16_t *rawData;
  uint8_t dataLength;
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

// Logic variables
bool receiveMode = false;
bool prevModeBtnState = HIGH;

class BluetoothServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* bleServer) {
      #ifdef DEBUG
        Serial.println("Bluetooth connected");
      #endif

      bleDeviceConnected = true;
    }

    void onDisconnect(BLEServer* bleServer) {
      #ifdef DEBUG
        Serial.println("Bluetooth disconnected");
      #endif

      bleDeviceConnected = false;
    }
};

class BluetoothCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *characteristic) {
      std::string rxValue = characteristic -> getValue();

      const char* data = rxValue.c_str();
        
      #ifdef DEBUG
        Serial.println(data);
      #endif

      wifiInfo.clear();
      deserializeJson(wifiInfo, data);

      mqttClient.unsubscribe(TOPIC_CONN_IN);
      WiFi.disconnect();

      wifiDeviceConnected = DISCONNECTED;

      #ifdef DEBUG
        Serial.println("Cleared existing connections");
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

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char* buff_p = new char[length];
  for (int i = 0; i < length; i++) {
    buff_p[i] = (char) payload[i];
  }
  buff_p[length] = '\0';


  #ifdef DEBUG
    Serial.print(F("Message arrived [")); Serial.print(F(topic)); Serial.print(F("] "));

    Serial.println(buff_p);

    drawRemote();
    tft.setTextSize(TEXT_SIZE_S);
    tft.setCursor(0, TFT_WIDTH - 2); // width is height :)
    tft.print(F("MQTT: ")); tft.print(F(buff_p));
  #endif

  // A command sent from the app
  if (strcmp(topic, TOPIC_APP_COMMAND) == 0) {
    Command command = deserializeCommand(buff_p);

    emitData(command);

    delete[] command.rawData;
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
  } else {
    #ifdef DEBUG
      Serial.println(F("Attempting MQTT connection..."));
    #endif
    
    // Create a random client ID so that it does 
    // not clash with other subscribed clients
    const String clientId = UUID_PREFIX + String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str())) {
      #ifdef DEBUG
        Serial.println(F("Connected to MQTT server"));

        mqttClient.publish(TOPIC_CONN_OUT, "Publish test WIO");
      #endif

      mqttClient.subscribe(TOPIC_CONN_IN); // topic to receive "pongs" from the app
      mqttClient.subscribe(TOPIC_APP_COMMAND); // topic to receive IR commands from the app
    } else {
      #ifdef DEBUG
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
    #ifdef DEBUG
      if(wifiDeviceConnected != CONNECTED) {
        Serial.println("Connected to " + WiFi.SSID());
        Serial.print(F("IP address: "));
        Serial.println(WiFi.localIP());
      }
    #endif

    wifiDeviceConnected = CONNECTED;

    updateMQTT();
  } else {
    const char *ssid = wifiInfo["ssid"];

    if(wifiDeviceConnected != CONNECTING) {  
      #ifdef DEBUG
        Serial.print(F("Connecting to "));
        Serial.println(F(ssid));
      #endif

      wifiDeviceConnected = CONNECTING;

      WiFi.begin(ssid, wifiInfo["password"], 0, stringToMAC(wifiInfo["bssid"]));
    }
  }
}

int getButtonPressed(){
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

/* Expected format (may also contain more keys):
  {
    "dataLength":<length>,
    "rawData":[<byte0>,<byte1>,...]
  }
*/
Command deserializeCommand(const char* jsonString) {
  JsonDocument* doc = new JsonDocument;
  deserializeJson(*doc, jsonString);
  
  const uint8_t dataLength = (*doc)["dataLength"];
  JsonArray rawDataJson = (*doc)["rawData"];

  uint16_t *rawData = new uint16_t[dataLength];
  for (uint8_t i = 0; i < dataLength; i++) {
    rawData[i] = rawDataJson[i];
  }

  delete doc;
  return {rawData, dataLength};
}

void drawRemote(){
  if (receiveMode) {
    tft.fillScreen(TFT_WHITE);
	  tft.setTextColor(TFT_BLACK);
    tft.setTextSize(TEXT_SIZE_M);
	  tft.drawString(F("Recording IR"), CENTER_X, CENTER_Y);
  } else {
    // Screen background
    tft.fillScreen(BACKGROUND_COLOR);

    // Middle button 
    tft.drawCircle(CENTER_X, CENTER_Y, CIRCLE_RADIUS + 2, OUTER_CIRCLE_COLOR);
    tft.fillCircle(CENTER_X, CENTER_Y, CIRCLE_RADIUS, CIRCLE_COLOR);
    tft.setTextSize(TEXT_SIZE_L);
    tft.setTextColor(TFT_WHITE);
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
  }
}

void emitData(const Command& command){
	if (command.rawData != nullptr){
		emitter.send(command.rawData, command.dataLength, CARRIER_FREQUENCY_KHZ);

    #ifdef DEBUG
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
  receiveMode = !receiveMode;

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

void receive(){
	if (!canMapButtons()) {
		switchMode();

		return;
	}

	receiver.enableIRIn();

	if (receiver.getResults()){
		const uint8_t dataLength = recvGlobal.recvLength;
		uint16_t *rawData = new uint16_t[dataLength];

		for (uint8_t i = 1; i < dataLength; i++) {
			rawData[i - 1] = recvGlobal.recvBuffer[i];
		}

		rawData[dataLength - 1] = 1000; // Arbitrary trailing space
    const Command recCommand = {rawData, dataLength};
    
    // Save the signal to a button
    tft.setTextSize(TEXT_SIZE_S);
    tft.drawString(F("Received. Press a button to save."), CENTER_X, CENTER_Y + 20);

    int chosenButton;
    
    do {
      chosenButton = getButtonPressed();

      if (digitalRead(MODE_BTN) == LOW){
			  switchMode();
		  } else {
        commandMap[chosenButton] = recCommand; // Write the received command to the map
      }
    } while(chosenButton == -1); // Wait for a button press

    drawRemote();
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
    int pressed = getButtonPressed();
  
    if (pressed != -1) {
      const Command command = commandMap[pressed];
      
			if (command.dataLength != 0){
        emitData(command);

        digitalWrite(MO_PIN, HIGH); // Vibrate if data sent

        #ifdef DEBUG // Flash a circle next to the pressed button label
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

				delay(250);
				digitalWrite(MO_PIN, LOW);

        #ifdef DEBUG
          tft.fillRect(0, 0, 10, 124, TFT_BLACK); // Erase the circle
        #endif
			}
    }	
	}

  // DEBUG: print configured buttons on screen
  #ifdef DEBUG
    tft.setTextSize(TEXT_SIZE_M);

    if (commandMap[POWER_BTN].dataLength != 0) {
      tft.drawString(F("POWER"), 20, 20);
    } else if (commandMap[UP_BTN].dataLength != 0) {
      tft.drawString(F("UP"), 20, 40);
    } else if (commandMap[LEFT_BTN].dataLength != 0) {
      tft.drawString(F("LEFT"), 20, 60);
    } else if (commandMap[RIGHT_BTN].dataLength != 0) {
      tft.drawString(F("RIGHT"), 20, 80);
    } else if (commandMap[DOWN_BTN].dataLength != 0) {
      tft.drawString(F("DOWN"), 20, 100);
    } else if (commandMap[PRESS_BTN].dataLength != 0) {
      tft.drawString(F("OK"), 20, 120);
    }
  #endif
  delay(50); // Slow down the loop
}