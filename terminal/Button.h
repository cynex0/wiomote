#ifndef BUTTON_H
#define BUTTON_H

class Button {
private:
  const int pin;
  const char* name;
  bool prevState;
public:
  Button(const int, const char*);
  bool isPressed();
  const char* getName();
};

#endif