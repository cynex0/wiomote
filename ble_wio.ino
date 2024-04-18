#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

const int SCREEN_ROTATION = 3;

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

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
    }
};

void setup() {
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

  tft.begin();
  tft.setRotation(SCREEN_ROTATION);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor((320 - tft.textWidth("Waiting for input...")) / 2, 120);
  tft.print("Waiting for input...");
}

void loop() {
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // Give the bluetooth stack the chance to get things ready
        pServer -> startAdvertising(); // Restart advertising
        oldDeviceConnected = deviceConnected;
    }

    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
}