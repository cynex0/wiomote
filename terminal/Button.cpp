#include "Button.h"
#include <Arduino.h>

Button::Button(const int pin_, const char* name_):
  pin(pin_),
  name(name_),
  prevState(HIGH) 
{
  pinMode(pin, INPUT_PULLUP); // LOW = pressed, HIGH = released
}

bool Button::isPressed() {
  // Prevent holding the button
  bool pressed = false;
  bool currState = digitalRead(pin);
  if (currState != prevState && currState == LOW) {
    pressed = true;
  }
  
  prevState = currState;
  return pressed;
}

const char* Button::getName() {
  return name;
}