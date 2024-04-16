#include <string.h>

#include <Dictionary.h>
#include <DictionaryDeclarations.h>

#include <IRLibSendBase.h>
#include <IRLib_HashRaw.h>
#include <IRLibCombo.h>
#include <IRLibRecvPCI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft;						 // Initializing TFT LCD library
TFT_eSprite spr = TFT_eSprite(&tft); // Initializing the buffer

IRsendRaw emitter;		  // Initializing IR Emitter
IRrecvPCI receiver(BCM3); // Initializing IR Receiver

const int CARRIER_FREQUENCY_KHZ = 38;
const int X = 160;
const int Y = 120;
const int RADIUS = 50;
const int COLOR = TFT_BLUE;

bool receiveMode = false;

Dictionary &commandMap = *(new Dictionary(6)); // char* -> char* map, maps buttons to commands

int MoPin = D0;

struct Command {
  uint16_t *rawData;
  uint8_t dataLength;
};

void resetUI(){
	// Draw action button
	spr.fillSprite(TFT_BLACK);
	spr.setTextSize(2);
	spr.setTextColor(TFT_WHITE);
	spr.drawCircle(X, Y, RADIUS, COLOR);
	spr.drawString("EMIT", 137, 115);

	// Push sprite changes
	spr.pushSprite(0, 0);
}

void setup(){
	Serial.begin(9600); // Start serial

	// Set up input
	pinMode(WIO_5S_PRESS, INPUT_PULLUP);
	pinMode(WIO_5S_UP, INPUT_PULLUP);
  pinMode(WIO_5S_DOWN, INPUT_PULLUP);
  pinMode(WIO_5S_LEFT, INPUT_PULLUP);
  pinMode(WIO_5S_RIGHT, INPUT_PULLUP);
	pinMode(WIO_KEY_A, INPUT_PULLUP);
	pinMode(WIO_KEY_B, INPUT_PULLUP);
	pinMode(WIO_KEY_C, INPUT_PULLUP);

	pinMode( MoPin, OUTPUT );

  // set up buttons-commands map
  commandMap("PRESS", nullptr);
  commandMap("UP", nullptr);
  commandMap("DOWN", nullptr);
  commandMap("LEFT", nullptr);
  commandMap("RIGHT", nullptr);
  commandMap("POWER", nullptr);

	tft.begin();							 // LCD initialization
	tft.setRotation(3);						 // Setting LCD rotation
	spr.createSprite(TFT_HEIGHT, TFT_WIDTH); // Creating the buffer

	resetUI();
}

char* buttonPressed(){
  char *out = nullptr;
  if (digitalRead(WIO_5S_PRESS) == LOW) out = "PRESS";
  else if (digitalRead(WIO_5S_UP) == LOW) out = "UP";
  else if (digitalRead(WIO_5S_DOWN) == LOW) out = "DOWN";
  else if (digitalRead(WIO_5S_LEFT) == LOW) out = "LEFT";
  else if (digitalRead(WIO_5S_RIGHT) == LOW) out = "RIGHT";
  else if (digitalRead(WIO_KEY_A) == LOW) out = "POWER";
  return out;
}

String serializeCommand(const Command& cmd){
  // Format : "len, timing_1, timing_2, ..., timing_len"
  String serialized = "";
  serialized += String(cmd.dataLength) + ","; // Append data length to the string
  
  // Append each timing from rawData array to the string
  for (int i = 0; i < cmd.dataLength; i++) {
    serialized += String(cmd.rawData[i]);
    if (i < cmd.dataLength - 1) {
      serialized += ",";
    }
  }
  
  return serialized;
}

Command deserializeCommand(const String& serialized) {
  // Format : "len, timing_1, timing_2, ..., timing_len"
  Command cmd;
  if (serialized == nullptr) {
    cmd = {nullptr, 0}; // if null string received, return null command
    return cmd;
  } 
  
  int commaIndex = serialized.indexOf(',');  // get index of the first comma
  cmd.dataLength = serialized.substring(0, commaIndex).toInt(); // get data length from the serialized string
  
  cmd.rawData = new uint16_t[cmd.dataLength]; // allocate memory for rawData array
  commaIndex++; // move the index past the comma

  // Extract each timing of rawData array from the string
  for (uint8_t i = 0; i < cmd.dataLength; i++) {
    int nextCommaIndex = serialized.indexOf(',', commaIndex);
    if (nextCommaIndex == -1) {
      nextCommaIndex = serialized.length();
    }
    cmd.rawData[i] = serialized.substring(commaIndex, nextCommaIndex).toInt();
    commaIndex = nextCommaIndex + 1;
  }
  
  return cmd;
}

void sendData(uint16_t *data, uint8_t dataLength){
	if (data != nullptr){
		emitter.send(data, dataLength, CARRIER_FREQUENCY_KHZ); // Pass the array, array length, carrier frequency

		tft.fillCircle(X, Y, RADIUS, COLOR); // Fill circle with color
		delay(200);

		spr.pushSprite(0, 0); // Clear UI changes
		delay(50);
	}
}

void switchToReceive(){
	receiveMode = true;

	receiver.enableIRIn();

	spr.fillSprite(TFT_WHITE); // Fill screen with white
	spr.setTextColor(TFT_BLACK);
	spr.drawString("RECORD", 127, 115);
	spr.pushSprite(0, 0);
}


void receive(){
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
    spr.drawString("Received. Press button to save.", 10, 225);
    spr.pushSprite(0, 0);
    char *chosenButton;
    do {
      chosenButton = buttonPressed();
    } while(chosenButton == nullptr); // wait for a button press
    commandMap(chosenButton, serializeCommand(recCommand)); // write the received command to the map

		resetUI();
		receiver.disableIRIn();
		Serial.println("Recorded data.");
		receiveMode = false;
	}
}

void loop(){
	if (receiveMode){
		receive();
	} else { // Detecting button presses
    if (digitalRead(WIO_KEY_C) == LOW){
			switchToReceive(); // Enter receive mode
		}

    char* pressed = buttonPressed();
    if (pressed != nullptr) {
      Command command = deserializeCommand(commandMap[pressed]);

			if (command.rawData != nullptr){
        sendData(command.rawData, command.dataLength);
				digitalWrite(MoPin, HIGH); // vibrate if data sent
				delay(250);
				digitalWrite(MoPin, LOW);
			}
    }		
	}
}