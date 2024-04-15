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

uint16_t *rawData = nullptr;
uint8_t dataLength = 0;
bool receiveMode = false;

int MoPin = D0;

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
	pinMode(WIO_5S_PRESS, INPUT);
	pinMode(WIO_KEY_A, INPUT);
	pinMode(WIO_KEY_B, INPUT);
	pinMode(WIO_KEY_C, INPUT);

	pinMode( MoPin, OUTPUT );

	tft.begin();							 // LCD initialization
	tft.setRotation(3);						 // Setting LCD rotation
	spr.createSprite(TFT_HEIGHT, TFT_WIDTH); // Creating the buffer

	resetUI();
}

void sendData(uint16_t *data){
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
		dataLength = recvGlobal.recvLength;
		rawData = new uint16_t[dataLength];

		for (bufIndex_t i = 1; i < dataLength; i++){
			rawData[i - 1] = recvGlobal.recvBuffer[i];
		}
		rawData[dataLength - 1] = 1000; // Arbitrary trailing space

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
		if (digitalRead(WIO_5S_PRESS) == LOW){
			sendData(rawData);

			if (rawData != nullptr){ // Vibrate if data is present
				digitalWrite(MoPin, HIGH);
				delay(250);
				digitalWrite(MoPin, LOW);
			}
		}

		if (digitalRead(WIO_KEY_A) == LOW ||
			digitalRead(WIO_KEY_B) == LOW ||
			digitalRead(WIO_KEY_C) == LOW){

			switchToReceive(); // Enter receive mode
		}
	}
}