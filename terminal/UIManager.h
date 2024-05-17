#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <TFT_eSPI.h>

#define FRAME_DELAY 20

enum class TerminalMode {CLONE, EMIT, CONFIG};

class UIManager {
private:
  TFT_eSPI tft;
  bool bltConnectedPrev;
  bool wifiConnectedPrev;
  
  void drawCloneModeUI();
  void drawEmitModeUI();
  void drawWiFiIcon(bool);
  void drawBltIcon(bool);
  void redrawStatusIcons();

public:
  UIManager();

  void setup();
  void redraw(TerminalMode);
  void updateBltIconStatus(bool, TerminalMode);
  void updateWiFiIconStatus(bool, TerminalMode);
  void drawButtonSelected(const char*);
  void playReceiveSignalAnimation();
  void playEmitSignalAnimation();
};

#endif