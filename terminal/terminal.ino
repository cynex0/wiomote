#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <TFT_eSPI.h>
#include <SPI.h> // drawing on screen
#include <rpcWiFi.h> // for wifi connection
#include <PubSubClient.h> // for MQTT

// Terminal commands to check if it works via mosquitto
// mosquitto_sub -v -h 'broker.hivemq.com' -p 1883 -t 'dit113/testwio12321Out'
// mosquitto_pub -h 'broker.hivemq.com' -p 1883 -t "dit113/testwio12321In" -m "message to terminal"

TFT_eSPI tft = TFT_eSPI();           // Initializing TFT LCD library
TFT_eSprite spr = TFT_eSprite(&tft); // Initializing the buffer

// Screen-related constants
#define X 160
#define Y 120
#define RADIUS 50
#define COLOR TFT_BLUE
#define SCREEN_ROTATION 3

// BLE variables
BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

// MQTT and Wi-Fi constants
#define mqtt_server "broker.hivemq.com"
#define MQTT_PORT 1883
#define topicOut "dit113/testwio12321Out"
#define topicIn "dit113/testwio12321In"


// MQTT variables
bool MQTTconnected = false;
WiFiClient wioClient;
PubSubClient client(wioClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void connectToWifi(const char *ssid, const char *password) {
  // networkInfo must contain ssid, bssid, password

  delay(10);

  tft.setTextSize(2);
  tft.setCursor((320 - tft.textWidth("Connecting to Wi-Fi..")) / 2, 120);
  tft.print("Connecting to Wi-Fi..");

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

  tft.fillScreen(TFT_BLACK);
  tft.setCursor((320 - tft.textWidth("Connected!")) / 2, 120);
  tft.print("Connected!");

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
  tft.fillScreen(TFT_BLACK);
  tft.setCursor((320 - tft.textWidth("MQTT Message")) / 2, 90);
  tft.print("MQTT Message: " );
  tft.setCursor((320 - tft.textWidth(msg_p)) / 2, 120);
  tft.print(msg_p); // Print receved payload
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

      tft.fillScreen(TFT_BLACK);
      tft.setCursor((320 - tft.textWidth("Waiting for input...")) / 2, 120);
      tft.print("Waiting for input...");
      tft.setCursor(0, 0);
      tft.print(data);

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

void setup() {
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  setupBLE();
  
  tft.begin();
  tft.setRotation(SCREEN_ROTATION);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor((320 - tft.textWidth("Waiting for input...")) / 2, 120);
  tft.print("Waiting for input...");
}

unsigned long lastPinged = 0;
void loop() {
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // Give the bluetooth stack the chance to get things ready
        pServer -> startAdvertising(); // Restart advertising
        oldDeviceConnected = deviceConnected;
    }

    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }

    if (client.connected()) 
      client.loop(); // receive messages

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
    delay(100);
}