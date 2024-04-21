#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <rpcWiFi.h> // for wifi connection
#include <PubSubClient.h> // for MQTT

#include <IRLibSendBase.h>
#include <IRLib_HashRaw.h>
#include <IRLibCombo.h>
#include <IRLibRecvPCI.h>
#include <TFT_eSPI.h>

// #define DEBUG // debugging mode

// Button indexes for the array acting as a map
#define BTN_COUNT       6
#define POWER_BTN_INDEX 0
#define UP_BTN_INDEX    1
#define RIGHT_BTN_INDEX 2
#define DOWN_BTN_INDEX  3
#define LEFT_BTN_INDEX  4
#define PRESS_BTN_INDEX 5

// Declaring UI Constants
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

#define BACKGROUND_COLOR TFT_BLACK  // Define screen color

// Defining buttons
#define UP_BTN        WIO_5S_UP
#define DOWN_BTN    WIO_5S_DOWN
#define LEFT_BTN    WIO_5S_LEFT
#define RIGHT_BTN  WIO_5S_RIGHT
#define PRESS_BTN     WIO_5S_PRESS
#define POWER_BTN  WIO_KEY_C
#define MODE_BTN  WIO_KEY_A

// Define button text, size and color
#define buttonText     "OK"
#define textSize          3
#define textColor TFT_WHITE

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

// MQTT and Wi-Fi constants
#define mqtt_server        "broker.hivemq.com"
#define MQTT_PORT                        1883
#define topicOut "wiomote/connection/terminal"
#define topicIn       "wiomote/connection/app"

#define CARRIER_FREQUENCY_KHZ 38


// Terminal commands to check if it works via mosquitto
// mosquitto_sub -v -h 'broker.hivemq.com' -p 1883 -t 'dit113/testwio12321Out'
// mosquitto_pub -h 'broker.hivemq.com' -p 1883 -t "dit113/testwio12321In" -m "message to terminal"

struct Command {
  uint16_t *rawData;
  uint8_t dataLength;
};

Command *commandMap = new Command[BTN_COUNT]; // maps buttons to commands

TFT_eSPI tft;           // Initializing TFT LCD library

IRsendRaw emitter;		  // Initializing IR Emitter
IRrecvPCI receiver(BCM3); // Initializing IR Receiver

// BLE variables
BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// MQTT variables
bool MQTTconnected = false;
WiFiClient wioClient;
PubSubClient client(wioClient);
long lastMsg = 0;
char msg[50];
int value = 0;

// logic variables
bool receiveMode = false;
bool modeBtnState;
bool prevModeBtnState = HIGH;

int MoPin = D0; // motor pin


void resetUI(){
  if (!receiveMode) {
    tft.fillScreen(BACKGROUND_COLOR);                                           // Drawing black background

    tft.drawCircle(CENTER_X, CENTER_Y, CIRCLE_RADIUS + 2, OUTER_CIRCLE_COLOR);  // Draw outline of middle circle
    tft.fillCircle(CENTER_X, CENTER_Y, CIRCLE_RADIUS, CIRCLE_COLOR);            // Colorfill middle circle

    // Set text size, position and color
    tft.setTextSize(textSize);
    tft.setTextColor(textColor);
  
    tft.drawString(buttonText, CENTER_X, CENTER_Y); // Draw center button text

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
  } else {
    tft.fillScreen(TFT_WHITE); // Fill screen with white
	  tft.setTextColor(TFT_BLACK);
    tft.setTextSize(2);
	  tft.drawString("Recording IR", CENTER_X, CENTER_Y);
  } 
}

void connectToWifi(const char *ssid, const char *password) {
  // networkInfo must contain ssid, bssid, password
  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password); // Connecting WiFi

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    WiFi.begin(ssid, password); // Connecting WiFi
  }

  Serial.println("");
  Serial.println("WiFi connected");

  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); // Display Local IP Address
}

void callback(char* topic, byte* payload, unsigned int length) {
  //tft.fillScreen(TFT_BLACK);
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  char buff_p[length];
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    buff_p[i] = (char)payload[i];
  }
  Serial.println();
  buff_p[length] = '\0';
  String msg_p = String(buff_p);
  
  resetUI();
  tft.setTextSize(1);
  tft.drawString("MQTT: " + msg_p, 0, TFT_WIDTH - 2); // width is height :)
}

void connectMQTT() {
  // Loop until reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "WioTerminal-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      MQTTconnected = true;
      // Once connected, publish an announcement...
      client.publish(topicOut, "wazzup");
      // ... and resubscribe
      client.subscribe(topicIn);
    } else {
      MQTTconnected = false;
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


class BluetoothServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class BluetoothCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic -> getValue();

      const char* data = rxValue.c_str();
        
      Serial.println(data);

      // tft.fillScreen(TFT_BLACK);
      // tft.setCursor((320 - tft.textWidth("Waiting for input...")) / 2, 120);
      // tft.print("Waiting for input...");
      // tft.setCursor(0, 0);
      // tft.print(data);

      JsonDocument networkInfo;
      deserializeJson(networkInfo, data);

      connectToWifi(networkInfo["ssid"], networkInfo["password"]);
      client.setServer(mqtt_server, MQTT_PORT); // Connect the MQTT Server
      client.setCallback(callback);
      connectMQTT();
    }
};

void setupBLE() {
  BLEDevice::init("WIOmote");  // Define device name

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer -> setCallbacks(new BluetoothServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                              CHARACTERISTIC_UUID_RX,
                              BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic -> setAccessPermissions(GATT_PERM_READ | GATT_PERM_WRITE);           

  pRxCharacteristic -> setCallbacks(new BluetoothCallbacks());

  // Start the service
  pService -> start();

  // Start advertising
  pServer -> startAdvertising();
}

int buttonPressed(){
  int out = -1;
  if (digitalRead(PRESS_BTN) == LOW) out = PRESS_BTN_INDEX;
  else if (digitalRead(UP_BTN) == LOW) out = UP_BTN_INDEX;
  else if (digitalRead(DOWN_BTN) == LOW) out = DOWN_BTN_INDEX;
  else if (digitalRead(LEFT_BTN) == LOW) out = LEFT_BTN_INDEX;
  else if (digitalRead(RIGHT_BTN) == LOW) out = RIGHT_BTN_INDEX;
  else if (digitalRead(POWER_BTN) == LOW) out = POWER_BTN_INDEX;
  return out;
}

void sendData(uint16_t *data, uint8_t dataLength){
	if (data != nullptr){
		emitter.send(data, dataLength, CARRIER_FREQUENCY_KHZ); // Pass the array, array length, carrier frequency

    Serial.println();
    Serial.print("Signal sent: ["); Serial.print(dataLength); Serial.print("]{");
    for (uint8_t i = 0; i < dataLength; i++) {
      Serial.print(data[i]);
      if (i != dataLength - 1) {
        Serial.print(", ");
      }
    }
    Serial.println("}");
	}
}

void switchMode(){
  receiveMode = !receiveMode;
  if (receiveMode) {
    // switch to emit
    receiver.enableIRIn();
  } else {
    receiver.disableIRIn();
  }
	resetUI();
}

boolean allButtonsMapped() {
	for (uint8_t i = 0; i < BTN_COUNT; i++) {
    if (commandMap[i].dataLength == 0) return false;
  }
	return true;
}

void receive(){
	if (allButtonsMapped()) {
		switchMode();
		return;
	}

	receiver.enableIRIn();

	if (receiver.getResults()){
		uint8_t dataLength = recvGlobal.recvLength;
		uint16_t *rawData = new uint16_t[dataLength];

		for (uint8_t i = 1; i < dataLength; i++){
			rawData[i - 1] = recvGlobal.recvBuffer[i];
		}
		rawData[dataLength - 1] = 1000; // Arbitrary trailing space
    Command recCommand = {rawData, dataLength};
    
    // Save the signal to a button
    tft.setTextSize(1);
    tft.drawString("Received. Press a button to save.", CENTER_X, CENTER_Y + 20);

    int chosenButton;
    do {
      chosenButton = buttonPressed();
      if (digitalRead(MODE_BTN) == LOW){
			  switchMode();
		  }
    } while(chosenButton == -1); // wait for a button press
    commandMap[chosenButton] = recCommand; // write the received command to the map
    resetUI();
	}
}

void setup() {
  Serial.begin(9600); // Start serial
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

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

	pinMode(MoPin, OUTPUT); 

  // Initialize commands "map"
  for(uint8_t i = 0; i < BTN_COUNT; i++){
    commandMap[i].rawData = new uint16_t[0];
	  commandMap[i].dataLength = 0;
  }

  // Screen setup
  tft.begin(); // LCD initialization
  tft.setRotation(SCREEN_ROTATION);
  tft.setTextDatum(MC_DATUM);
	resetUI();
}

unsigned long lastPinged = 0;
void loop() {
  // BLE connection
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // Give the bluetooth stack the chance to get things ready
    pServer -> startAdvertising(); // Restart advertising
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
  
  // MQTT
  if (client.connected()) 
    client.loop(); // receive messages

  // MQTT publish debug
  #ifdef DEBUG
  if (millis() - lastPinged > 5000) {
    // try publishing every 5 seconds
    if (MQTTconnected) {
      client.publish(topicOut, "ping"); // ping every 5 seconds
    } else if (WiFi.status() == WL_CONNECTED) {
      Serial.println("No MQTT connection");
    } else {
      Serial.println("No WiFi connection");
    }
    lastPinged = millis();
  }
  #endif
  
  // Mode button logic
  modeBtnState = digitalRead(MODE_BTN);
  if (modeBtnState != prevModeBtnState) {
    if (modeBtnState == LOW){
		  switchMode();
    }
    delay(10); // prevent button bouncing
  }
  prevModeBtnState = modeBtnState;


	if (receiveMode){
		receive();
	} else { // Detecting button presses
    int pressed = buttonPressed();
    if (pressed != -1) {
      Command command = commandMap[pressed];

			if (command.dataLength != 0){
        sendData(command.rawData, command.dataLength);
        digitalWrite(MoPin, HIGH); // vibrate if data sent

        // DEBUG: flash a circle next to the pressed button label
        #ifdef DEBUG
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
				digitalWrite(MoPin, LOW);

        #ifdef DEBUG
        tft.fillRect(0, 0, 10, 124, TFT_BLACK); // erase the circle
        #endif
			}
    }		
	}

  // DEBUG: print configured buttons on screen
  #ifdef DEBUG
  tft.setTextSize(2);
  if (commandMap[POWER_BTN].dataLength != 0)
    tft.drawString("POWER", 20, 20);
  if (commandMap[UP_BTN].dataLength != 0)
    tft.drawString("UP", 20, 40);
  if (commandMap[LEFT_BTN].dataLength != 0)
    tft.drawString("LEFT", 20, 60);
  if (commandMap[RIGHT_BTN].dataLength != 0)
    tft.drawString("RIGHT", 20, 80);
  if (commandMap[DOWN_BTN].dataLength != 0)
    tft.drawString("DOWN", 20, 100);
  if (commandMap[PRESS_BTN].dataLength != 0)
    tft.drawString("OK", 20, 120);
  #endif
}