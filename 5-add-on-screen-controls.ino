//#include <IRLib_HashRaw.h>
//#include <IRLibCombo.h> 
//#include <IRLibRecvPCI.h>          // IR receiver libraries

#include <TFT_eSPI.h>                // LCD library

TFT_eSPI tft;                             // Initializing tft LCD library
TFT_eSprite sprite = TFT_eSprite(&tft);   // Initializing buffer

// Declaring UI Constants
const int CIRCLE_COLOR = TFT_BLUE;
const int OUTER_CIRCLE_COLOR = TFT_WHITE;
const int CIRCLE_RADIUS = 45;
const int CENTER_X = 160;           // Middle point of screen X-axis
const int CENTER_Y = 120;           // Middle point of screen Y-axis

const int ARROW_TOP_OFFSET = 100;   // Distance from middle to the top of the arrows
const int ARROW_BASE_OFFSET = 60;   // Distance from middle to bottom sides of arrows
const int ARROW_LENGTH = ARROW_TOP_OFFSET - ARROW_BASE_OFFSET; // Value of arrow length
const int ARROW_COLOR = TFT_WHITE;

const int BACKGROUND_COLOR = TFT_BLACK;

// Defining buttons
const int upButton = WIO_5S_UP;
const int downButton = WIO_5S_DOWN;
const int leftButton = WIO_5S_LEFT;
const int rightButton = WIO_5S_RIGHT;
const int okButton = WIO_5S_PRESS;

// Define button text, size and color
const String buttonText = "OK";
const int textSize = 3;
const int textColor = TFT_WHITE;

void drawUI() {   // Method to draw the UI on screen

    tft.fillScreen(BACKGROUND_COLOR);   // Drawing black background

    tft.drawCircle(CENTER_X, CENTER_Y, CIRCLE_RADIUS + 2, OUTER_CIRCLE_COLOR);  // Draw outline of middle circle
    tft.fillCircle(CENTER_X, CENTER_Y, CIRCLE_RADIUS, CIRCLE_COLOR);            // Colorfill middle circle

    // Set text size, position and color
    tft.setTextSize(textSize);
    tft.setTextColor(textColor);
    tft.setTextDatum(MC_DATUM);
    
    tft.drawString(buttonText, CENTER_X, CENTER_Y);   // Draw center button text

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
}

void setup() {

    Serial.begin(9600);
    tft.begin();
    tft.setRotation(3);

    // Initialize button pins
    pinMode(upButton, INPUT); 
    pinMode(downButton, INPUT);
    pinMode(leftButton, INPUT);
    pinMode(rightButton, INPUT);
    pinMode(okButton, INPUT);

    drawUI(); // Draw the UI
}

void loop() {
    
    // Check button press and print message if pressed
    if (digitalRead(upButton) == LOW) {
    
        Serial.println("Up Button");
    }
    if (digitalRead(downButton) == LOW) {

        Serial.println("Down Button");
    }
    if (digitalRead(leftButton) == LOW) {

        Serial.println("Left Button");
    }
    if (digitalRead(rightButton) == LOW) {

        Serial.println("Right Button");
    }
    if (digitalRead(okButton) == LOW) {

        Serial.println("OK Button");
    }
}