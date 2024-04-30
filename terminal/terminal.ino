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
#define POWER_BTN_INDEX 0
#define UP_BTN_INDEX    1
#define RIGHT_BTN_INDEX 2
#define DOWN_BTN_INDEX  3
#define LEFT_BTN_INDEX  4
#define PRESS_BTN_INDEX 5

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
#define MQTT_SERVER         "broker.hivemq.com"
#define MQTT_PORT                         1883
#define TOPIC_OUT "wiomote/connection/terminal"
#define TOPIC_IN       "wiomote/connection/app"

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

      mqttClient.unsubscribe(TOPIC_IN);
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
  #ifdef DEBUG
    Serial.printf("Message arrived [%s] ", topic);

    char buff_p[length];
    for (int i = 0; i < length; i++) {
      Serial.print((char) payload[i]);
      buff_p[i] = (char) payload[i];
    }

    Serial.println();

    buff_p[length] = '\0';

    drawRemote();

    tft.setTextSize(TEXT_SIZE_S);
    tft.drawString("MQTT: " + String(buff_p), 0, TFT_WIDTH - 2); // width is height :)
  #endif
}

void setupMQTT() {
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
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
      Serial.println("Attempting MQTT connection...");
    #endif
    
    // Create a random client ID so that it does 
    // not clash with other subscribed clients
    String clientId = "WioTerminal-" + String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str())) {
      #ifdef DEBUG
        Serial.println("Connected to MQTT server");

        mqttClient.publish(TOPIC_OUT, "Publish test WIO");
      #endif

      mqttClient.subscribe(TOPIC_IN);
    } else {
      #ifdef DEBUG
        Serial.print("Failed to connect to MQTT server - rc=" + mqttClient.state());
      #endif
    }
  }
}

void WiFiEvent(WiFiEvent_t event){
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
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
      }
    #endif

    wifiDeviceConnected = CONNECTED;

    updateMQTT();
  } else {
    const char *ssid = wifiInfo["ssid"];

    if(wifiDeviceConnected != CONNECTING) {  
      #ifdef DEBUG
        Serial.printf("Connecting to %s...\n", ssid);
      #endif

      wifiDeviceConnected = CONNECTING;

      WiFi.begin(ssid, wifiInfo["password"], 0, stringToMAC(wifiInfo["bssid"]));
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

char* serializeCommand(const Command &cmd, const int button) {
  JsonDocument doc;
  char* out = new char[128]; // Buffer to hold the output
  size_t capacity;
  doc["dataLength"] = cmd.dataLength;

  JsonArray data = doc["data"].to<JsonArray>();
  for(int i = 0; i < cmd.dataLength; i++) {
    data.add(cmd.rawData[i]);
  }

  doc["button"] = getButtonName(button);
  serializeJson(doc, out, 128);
  return out;
}

void drawRemote(){
  if (receiveMode) {
    tft.fillScreen(TFT_WHITE);
	  tft.setTextColor(TFT_BLACK);
    tft.setTextSize(TEXT_SIZE_M);
	  tft.drawString("Select a button", CENTER_X, CENTER_Y);
  } else {
    // Screen background
    tft.fillScreen(BACKGROUND_COLOR);

    // Middle button 
    tft.drawCircle(CENTER_X, CENTER_Y, CIRCLE_RADIUS + 2, OUTER_CIRCLE_COLOR);
    tft.fillCircle(CENTER_X, CENTER_Y, CIRCLE_RADIUS, CIRCLE_COLOR);
    tft.setTextSize(TEXT_SIZE_L);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("OK", CENTER_X, CENTER_Y);

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

void emitData(uint16_t *data, uint8_t dataLength){
	if (data != nullptr){
		emitter.send(data, dataLength, CARRIER_FREQUENCY_KHZ);

    #ifdef DEBUG
      Serial.print("Signal sent: ["); Serial.print(dataLength); Serial.print("]{");

      for (uint8_t i = 0; i < dataLength; i++) {
        Serial.print(data[i]);

        if (i != dataLength - 1) {
          Serial.print(", ");
        }
      }

      Serial.println("}");
    #endif
	}
}

void switchMode(){
  receiveMode = !receiveMode;
  receiveMode ? receiver.enableIRIn() : receiver.disableIRIn();

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

int chosenButton = -1;
void receive(){
	if (!canMapButtons()) {
		switchMode();
		return;
	}

  int pressedButton = getButtonPressedIndex();
  if (pressedButton != -1 && pressedButton != chosenButton) {
    chosenButton = pressedButton;
    tft.fillRect(0, CENTER_Y - tft.fontHeight(TEXT_SIZE_M)/2, TFT_HEIGHT, tft.fontHeight(TEXT_SIZE_M), TFT_WHITE);
    tft.setTextSize(TEXT_SIZE_M);
    char message[16];
    sprintf(message, "%s selected!", getButtonName(chosenButton));
    tft.drawString(message, CENTER_X, CENTER_Y);
    tft.setTextSize(TEXT_SIZE_S);
    tft.drawString("Waiting for IR.", CENTER_X, CENTER_Y + 20);
  }
  
  // wait until a button is pressed
  if (chosenButton != -1) {
    receiver.enableIRIn();
	  if (receiver.getResults()){
		  uint8_t dataLength = recvGlobal.recvLength;
		  uint16_t *rawData = new uint16_t[dataLength];

      for (uint8_t i = 1; i < dataLength; i++) {
        rawData[i - 1] = recvGlobal.recvBuffer[i];
      }

      rawData[dataLength - 1] = 1000; // Arbitrary trailing space
      
      Command recCommand = {rawData, dataLength};
      commandMap[chosenButton] = recCommand; // Write the received command to the map
      
      mqttClient.publish("wiomote/command/cloning", serializeCommand(recCommand, chosenButton));

      tft.drawString("Recorded!", CENTER_X, CENTER_Y + 40);
      delay(50);
      drawRemote(); // Reset the UI
      chosenButton = -1;
    }
  } 
}

void setup() {
  Serial.begin(9600); // Start serial

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
    int pressed = getButtonPressedIndex();

    if (pressed != -1) {
      Command command = commandMap[pressed];

			if (command.dataLength != 0){
        emitData(command.rawData, command.dataLength);

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
      tft.drawString("POWER", 20, 20);
    } else if (commandMap[UP_BTN].dataLength != 0) {
      tft.drawString("UP", 20, 40);
    } else if (commandMap[LEFT_BTN].dataLength != 0) {
      tft.drawString("LEFT", 20, 60);
    } else if (commandMap[RIGHT_BTN].dataLength != 0) {
      tft.drawString("RIGHT", 20, 80);
    } else if (commandMap[DOWN_BTN].dataLength != 0) {
      tft.drawString("DOWN", 20, 100);
    } else if (commandMap[PRESS_BTN].dataLength != 0) {
      tft.drawString("OK", 20, 120);
    }
  #endif
}