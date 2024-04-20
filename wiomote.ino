#include <IRLibSendBase.h>
#include <IRLib_HashRaw.h>
#include <IRLibCombo.h>
#include <IRLibRecvPCI.h>
#include <TFT_eSPI.h>

#define BTN_COUNT 6
#define POWER_BTN 0
#define UP_BTN    1
#define RIGHT_BTN 2
#define DOWN_BTN  3
#define LEFT_BTN  4
#define PRESS_BTN 5
#define MODE_BTN  WIO_KEY_C

// Declaring UI Constants
#define CIRCLE_COLOR        TFT_BLUE
#define OUTER_CIRCLE_COLOR TFT_WHITE
#define CIRCLE_RADIUS             45
#define CENTER_X                 160  // Middle point of screen X-axis
#define CENTER_Y                 120  // Middle point of screen Y-axis

#define ARROW_TOP_OFFSET  100  // Distance from middle to the top of the arrows
#define ARROW_BASE_OFFSET  60  // Distance from middle to bottom sides of arrows
#define ARROW_LENGTH       40  // Value of arrow length
#define ARROW_COLOR TFT_WHITE

#define BACKGROUND_COLOR TFT_BLACK  // Define screen color

// Defining buttons
#define upButton        WIO_5S_UP
#define downButton    WIO_5S_DOWN
#define leftButton    WIO_5S_LEFT
#define rightButton  WIO_5S_RIGHT
#define okButton     WIO_5S_PRESS

// Define button text, size and color
#define buttonText     "OK"
#define textSize          3
#define textColor TFT_WHITE


TFT_eSPI tft;						 // Initializing TFT LCD library
TFT_eSprite spr = TFT_eSprite(&tft); // Initializing the buffer

IRsendRaw emitter;		  // Initializing IR Emitter
IRrecvPCI receiver(BCM3); // Initializing IR Receiver


const int CARRIER_FREQUENCY_KHZ = 38;

bool receiveMode = false;

int MoPin = D0;

// states for mode switch button to prevent hold-down spam
bool modeBtnState;
bool prevModeBtnState = HIGH;

struct Command {
  uint16_t *rawData;
  uint8_t dataLength;
};


Command *commandMap = new Command[BTN_COUNT]; // maps buttons to commands

void resetUI(){

    spr.fillSprite(BACKGROUND_COLOR);                                           // Drawing black background

    spr.drawCircle(CENTER_X, CENTER_Y, CIRCLE_RADIUS + 2, OUTER_CIRCLE_COLOR);  // Draw outline of middle circle
    spr.fillCircle(CENTER_X, CENTER_Y, CIRCLE_RADIUS, CIRCLE_COLOR);            // Colorfill middle circle

    // Set text size, position and color
    spr.setTextSize(textSize);
    spr.setTextColor(textColor);
    
    spr.drawString(buttonText, CENTER_X, CENTER_Y); // Draw center button text

    // Draw top arrow
    spr.drawLine(CENTER_X, CENTER_Y - ARROW_TOP_OFFSET, CENTER_X + ARROW_LENGTH, CENTER_Y - ARROW_BASE_OFFSET, ARROW_COLOR);
    spr.drawLine(CENTER_X, CENTER_Y - ARROW_TOP_OFFSET, CENTER_X - ARROW_LENGTH, CENTER_Y - ARROW_BASE_OFFSET, ARROW_COLOR);

    // Draw right arrow
    spr.drawLine(CENTER_X + ARROW_TOP_OFFSET, CENTER_Y, CENTER_X + ARROW_BASE_OFFSET, CENTER_Y + ARROW_LENGTH, ARROW_COLOR);
    spr.drawLine(CENTER_X + ARROW_TOP_OFFSET, CENTER_Y, CENTER_X + ARROW_BASE_OFFSET, CENTER_Y - ARROW_LENGTH, ARROW_COLOR);

    // Draw bottom arrow
    spr.drawLine(CENTER_X, CENTER_Y + ARROW_TOP_OFFSET, CENTER_X - ARROW_LENGTH, CENTER_Y + ARROW_BASE_OFFSET, ARROW_COLOR);
    spr.drawLine(CENTER_X, CENTER_Y + ARROW_TOP_OFFSET, CENTER_X + ARROW_LENGTH, CENTER_Y + ARROW_BASE_OFFSET, ARROW_COLOR);

    // Draw left arrow
    spr.drawLine(CENTER_X - ARROW_TOP_OFFSET, CENTER_Y, CENTER_X - ARROW_BASE_OFFSET, CENTER_Y - ARROW_LENGTH, ARROW_COLOR);
    spr.drawLine(CENTER_X - ARROW_TOP_OFFSET, CENTER_Y, CENTER_X - ARROW_BASE_OFFSET, CENTER_Y + ARROW_LENGTH, ARROW_COLOR);

	  // Push sprite changes
	  spr.pushSprite(0, 0);
}

void setup(){
	Serial.begin(9600); // Start serial

	// Set up input
	pinMode(okButton, INPUT_PULLUP);
	pinMode(upButton, INPUT_PULLUP);
  pinMode(downButton, INPUT_PULLUP);
  pinMode(leftButton, INPUT_PULLUP);
  pinMode(rightButton, INPUT_PULLUP);
	pinMode(WIO_KEY_A, INPUT_PULLUP);
	pinMode(WIO_KEY_B, INPUT_PULLUP);
	pinMode(WIO_KEY_C, INPUT_PULLUP);

	pinMode( MoPin, OUTPUT );

	for(uint8_t i = 0; i < BTN_COUNT; i++){
    commandMap[i].rawData = new uint16_t[0];
	  commandMap[i].dataLength = 0;
  }


	tft.begin();							     // LCD initialization
	tft.setRotation(3);						 // Setting LCD rotation
	spr.createSprite(TFT_HEIGHT, TFT_WIDTH); // Creating the buffer
  spr.setTextDatum(MC_DATUM);

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
  if (receiveMode) {
    // switch to emit
		resetUI();
		receiver.disableIRIn();
  } else {
    receiver.enableIRIn();

	  spr.fillSprite(TFT_WHITE); // Fill screen with white
	  spr.setTextColor(TFT_BLACK);
    spr.setTextSize(2);
	  spr.drawString("RECORD", CENTER_X, CENTER_Y);
  	spr.pushSprite(0, 0);
  }

	receiveMode = !receiveMode;
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
    spr.drawString("Received. Press button to save.", 10, 225);
    spr.pushSprite(0, 0);
    int chosenButton;
    do {
      chosenButton = buttonPressed();
      if (digitalRead(MODE_BTN) == LOW){
			  switchMode();
		  }
    } while(chosenButton == -1); // wait for a button press
    commandMap[chosenButton] = recCommand; // write the received command to the map
	}
}

void loop(){
  modeBtnState = digitalRead(MODE_BTN);
  if (modeBtnState != prevModeBtnState) {
    if (modeBtnState == LOW){
		  switchMode();
    }
    delay(50); // prevent button bouncing
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
        switch(pressed) {
          case POWER_BTN:
            spr.fillCircle(5, 10, 4, TFT_GREEN);
            break;
          case UP_BTN:
            spr.fillCircle(5, 30, 4, TFT_GREEN);
            break;
          case LEFT_BTN:
            spr.fillCircle(5, 50, 4, TFT_GREEN);
            break;
          case RIGHT_BTN:
            spr.fillCircle(5, 70, 4, TFT_GREEN);
            break;
          case DOWN_BTN:
            spr.fillCircle(5, 90, 4, TFT_GREEN);
            break;
          case PRESS_BTN:
            spr.fillCircle(5, 110, 4, TFT_GREEN);
            break;
        }
        spr.pushSprite(0,0);
				delay(250);
				digitalWrite(MoPin, LOW);
        spr.fillRect(0, 0, 10, 120, TFT_BLACK);
        spr.pushSprite(0,0);
			}
    }		
	}

  // print configured buttons on screen for debug
  spr.setTextSize(2);
  if (commandMap[POWER_BTN].dataLength != 0)
    spr.drawString("POWER", 10, 0);
  if (commandMap[UP_BTN].dataLength != 0)
    spr.drawString("UP", 10, 20);
  if (commandMap[LEFT_BTN].dataLength != 0)
    spr.drawString("LEFT", 10, 40);
  if (commandMap[RIGHT_BTN].dataLength != 0)
    spr.drawString("RIGHT", 10, 60);
  if (commandMap[DOWN_BTN].dataLength != 0)
    spr.drawString("DOWN", 10, 80);
  if (commandMap[PRESS_BTN].dataLength != 0)
    spr.drawString("OK", 10, 100);
  
  spr.pushSprite(0,0);
  spr.setTextSize(textSize);
}