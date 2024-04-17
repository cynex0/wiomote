#include <IRLibSendBase.h>
#include <IRLib_HashRaw.h>
#include <IRLibCombo.h>
#include <IRLibRecvPCI.h>
#include <TFT_eSPI.h>

#define POWER_BTN 0
#define UP_BTN 1
#define RIGHT_BTN 2
#define DOWN_BTN 3
#define LEFT_BTN 4
#define PRESS_BTN 5

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

int MoPin = D0;

struct Command {
  uint16_t *rawData;
  uint8_t dataLength;
};


Command *commandMap = new Command[6]; // char* -> char* map, maps buttons to commands

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


	tft.begin();							 // LCD initialization
	tft.setRotation(3);						 // Setting LCD rotation
	spr.createSprite(TFT_HEIGHT, TFT_WIDTH); // Creating the buffer

	resetUI();
}

int buttonPressed(){
  int out = -1;
  if (digitalRead(WIO_5S_PRESS) == LOW) out = PRESS_BTN;
  else if (digitalRead(WIO_5S_UP) == LOW) out = UP_BTN;
  else if (digitalRead(WIO_5S_DOWN) == LOW) out = DOWN_BTN;
  else if (digitalRead(WIO_5S_LEFT) == LOW) out = LEFT_BTN;
  else if (digitalRead(WIO_5S_RIGHT) == LOW) out = RIGHT_BTN;
  else if (digitalRead(WIO_KEY_A) == LOW) out = POWER_BTN;
  return out;
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
    int chosenButton;
    do {
      chosenButton = buttonPressed();
    } while(chosenButton == -1); // wait for a button press
    commandMap[chosenButton] = recCommand; // write the received command to the map

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

    int pressed = buttonPressed();
    if (pressed != -1) {
      Command command = commandMap[pressed];

			if (command.rawData != nullptr){
        sendData(command.rawData, command.dataLength);
				digitalWrite(MoPin, HIGH); // vibrate if data sent
				delay(250);
				digitalWrite(MoPin, LOW);
			}
    }		
	}
}