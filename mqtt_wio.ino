#include <TFT_eSPI.h> // drawing on screen
#include <rpcWiFi.h> // for wifi connection
#include <PubSubClient.h> // for MQTT

// Terminal commands to check if it works via mosquitto
// mosquitto_sub -v -h 'broker.hivemq.com' -p 1883 -t 'dit113/testwio12321Out'
// mosquitto_pub -h 'broker.hivemq.com' -p 1883 -t "dit113/testwio12321In" -m "message to terminal"


TFT_eSPI tft;						 // Initializing TFT LCD library
TFT_eSprite spr = TFT_eSprite(&tft); // Initializing the buffer


const int X = 160;
const int Y = 120;
const int RADIUS = 50;
const int COLOR = TFT_BLUE;
const int SCREEN_ROTATION = 3;


const char* ssid = "***"; // WiFi Name or ssid
const char* password = "***";  // WiFi Password
const char* mqtt_server = "broker.hivemq.com";
const int MQTT_PORT = 1883;

const char* topicOut = "dit113/testwio12321Out";
const char* topicIn = "dit113/testwio12321In";

bool MQTTconnected = false;


WiFiClient wioClient;
PubSubClient client(wioClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup_wifi() {

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


void reconnect() {
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


void setup() {
  tft.begin();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(SCREEN_ROTATION);

  pinMode(WIO_KEY_C, INPUT);

  Serial.println();
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, MQTT_PORT); // Connect the MQTT Server
  client.setCallback(callback);

}

bool buttonUp = true;

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (digitalRead(WIO_KEY_C) == LOW && MQTTconnected && buttonUp){
    buttonUp = false;
    ++value;
    String messageStr = "button press #" + String(value);
    char* message = (char*) messageStr.c_str();
    client.publish(topicOut, message);
  }
  if (digitalRead(WIO_KEY_C) == HIGH){
    buttonUp = true;
  }
}
