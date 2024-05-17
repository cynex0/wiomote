#include "UIManager.h"
#include "UIConstants.h"

UIManager::UIManager() {
  bltConnectedPrev = false;
  wifiConnectedPrev = false;
}

void UIManager::setup() {
  tft.begin();
  tft.setRotation(SCREEN_ROTATION);
  tft.setTextDatum(MC_DATUM);
}

void UIManager::redraw(TerminalMode currMode) {
  if (currMode == TerminalMode::CLONE) {
    drawCloneModeUI();
  } 
  else if (currMode == TerminalMode::EMIT) {
    drawEmitModeUI();
    redrawStatusIcons();
  }
}

void UIManager::drawCloneModeUI() {
  tft.fillScreen(INVERTED_BG_COLOR);
  tft.setTextColor(INVERTED_TEXT_COLOR);
  tft.setTextSize(TEXT_SIZE_M);
  tft.drawString(F("Select a button"), CENTER_X, CENTER_Y);
}

void UIManager::drawButtonSelected(const char* buttonName) {
  tft.fillRect(0, CENTER_Y - tft.fontHeight(TEXT_SIZE_M)/2, TFT_HEIGHT, tft.fontHeight(TEXT_SIZE_M), TFT_WHITE); // Erase "Select a button"
    
  tft.setTextSize(TEXT_SIZE_M);
  tft.setTextColor(INVERTED_TEXT_COLOR);

  char message[16]; // Max length: 16 = 10 (" selected!") + 6 ("CUSTOM")
  sprintf(message, "%s selected!", buttonName);
  tft.drawString(message, CENTER_X, CENTER_Y);
    
  tft.setTextSize(TEXT_SIZE_S);
  tft.drawString(F("Waiting for IR."), CENTER_X, CENTER_Y + 20);
}

void UIManager::drawEmitModeUI() {
  // Screen background
  tft.fillScreen(DEFAULT_BG_COLOR);

  // Middle button 
  tft.drawCircle(CENTER_X, CENTER_Y, CIRCLE_RADIUS + 2, OUTER_CIRCLE_COLOR);
  tft.fillCircle(CENTER_X, CENTER_Y, CIRCLE_RADIUS, CIRCLE_COLOR);
  tft.setTextSize(TEXT_SIZE_L);
  tft.setTextColor(DEFAULT_TEXT_COLOR);;
  tft.drawString(F("OK"), CENTER_X, CENTER_Y);

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

  // Draw power icon
  tft.drawRect(5, 9, 21, 2, ARROW_COLOR);
  tft.drawCircle(15, 22, 6, ARROW_COLOR);
  tft.drawLine(15, 16, 15, 20, ARROW_COLOR);
}

void UIManager::drawConfigDebugUI(const char** configTexts, int length){
  tft.fillScreen(INVERTED_BG_COLOR);
  tft.setTextColor(INVERTED_TEXT_COLOR);
  tft.setTextSize(TEXT_SIZE_M);
  for(int i = 0; i < length; i++){
    tft.drawString(configTexts[i], 20, 20 + 20 * i); // Draw the labels for each config button
  }
}

void UIManager::drawConfigSkippedButton(int position) {
  tft.setTextSize(TEXT_SIZE_M);
  tft.setTextColor(TFT_RED);
  tft.drawString(F("SKIPPED"), 200, 20 + 20 * position); 
}

void UIManager::drawConfigRecordedButton(int position) {
  tft.setTextSize(TEXT_SIZE_M);
  tft.setTextColor(TFT_DARKGREEN);
  tft.drawString(F("RECORDED"), 200, 20 + 20 * position); // Draw "RECORDED" next to the labels
}

void UIManager::redrawStatusIcons() {
  // Draw wifi connection status icon
  drawWiFiIcon(wifiConnectedPrev);

  // Draw bluetooth connection status icon
  drawBltIcon(bltConnectedPrev);
}

void UIManager::updateBltIconStatus(bool bltConnected, TerminalMode mode) {
  // decide the color according to connection status and previous status so it doesn't loop
  if(bltConnected != bltConnectedPrev){ // if the bluetooth connection status is the same as before - do nothing
    if (mode == TerminalMode::EMIT) drawBltIcon(bltConnected); // if different - redraw the icon
    bltConnectedPrev = bltConnected;
  } 
}

void UIManager::updateWiFiIconStatus(bool wifiConnected, TerminalMode mode){
  // decide the color according to connection status and previous status so it doesn't loop
  if(wifiConnected != wifiConnectedPrev){ // if the wifi connection status is the same as before - do nothing
    if (mode == TerminalMode::EMIT) drawWiFiIcon(wifiConnected); // if different - draw the icon
    wifiConnectedPrev = wifiConnected;
  }
}

void UIManager::drawWiFiIcon(bool wifiDeviceConnected) {
  // empty the region for new icon
  tft.drawRect(WIFI_ICON_CIRCLE_X - WIFI_ICON_CIRCLE_MAX_RAD, WIFI_ICON_CIRCLE_Y - WIFI_ICON_CIRCLE_MAX_RAD, WIFI_ICON_CIRCLE_MAX_RAD * 2, WIFI_ICON_CIRCLE_MAX_RAD * 2, DEFAULT_BG_COLOR);

  // draw 3 circles
  for(int i = 0; i < 3; i++){
    tft.drawCircle(WIFI_ICON_CIRCLE_X, WIFI_ICON_CIRCLE_Y, WIFI_ICON_CIRCLE_MAX_RAD - WIFI_ICON_CIRCLE_RADIUS_DIFF * i, wifiDeviceConnected ? WIFI_ICON_ICON_COLOR_ON : WIFI_ICON_ICON_COLOR_OFF);
  }

  // use 2 triangles to mask the 3 circles into 3 arcs
  tft.fillTriangle(WIFI_ICON_CIRCLE_X - WIFI_ICON_CIRCLE_MAX_RAD, 
    WIFI_ICON_CIRCLE_Y + WIFI_ICON_CIRCLE_MAX_RAD, 
    WIFI_ICON_CIRCLE_X + WIFI_ICON_CIRCLE_MAX_RAD, 
    WIFI_ICON_CIRCLE_Y - WIFI_ICON_CIRCLE_MAX_RAD,
    WIFI_ICON_CIRCLE_X + WIFI_ICON_CIRCLE_MAX_RAD, 
    WIFI_ICON_CIRCLE_Y + WIFI_ICON_CIRCLE_MAX_RAD, 
    DEFAULT_BG_COLOR);
  
  tft.fillTriangle(WIFI_ICON_CIRCLE_X + WIFI_ICON_CIRCLE_MAX_RAD, 
    WIFI_ICON_CIRCLE_Y + WIFI_ICON_CIRCLE_MAX_RAD, 
    WIFI_ICON_CIRCLE_X - WIFI_ICON_CIRCLE_MAX_RAD, 
    WIFI_ICON_CIRCLE_Y - WIFI_ICON_CIRCLE_MAX_RAD, 
    WIFI_ICON_CIRCLE_X - WIFI_ICON_CIRCLE_MAX_RAD, 
    WIFI_ICON_CIRCLE_Y + WIFI_ICON_CIRCLE_MAX_RAD, 
    DEFAULT_BG_COLOR);
}

void UIManager::drawBltIcon(bool bleDeviceConnected) {
  // decide the color according to connection status
  uint32_t color;
  if (bleDeviceConnected){
    color = BLT_ICON_COLOR_ON;
  }
  else{
    color = BLT_ICON_COLOR_OFF;
  }

  // empty the region for new icon
  tft.drawRect(BLT_ICON_START_X, BLT_ICON_START_Y, BLT_ICON_WIDTH, BLT_ICON_HEIGHT, DEFAULT_BG_COLOR);

  // draw the bluetooth icon
  tft.drawLine(BLT_ICON_START_X, BLT_ICON_START_Y + BLT_ICON_HEIGHT * 3/4, BLT_ICON_START_X + BLT_ICON_WIDTH, BLT_ICON_START_Y + BLT_ICON_HEIGHT * 1/4, color);
  tft.drawLine(BLT_ICON_START_X + BLT_ICON_WIDTH, BLT_ICON_START_Y + BLT_ICON_HEIGHT * 1/4, BLT_ICON_START_X + BLT_ICON_WIDTH * 1/2, BLT_ICON_START_Y, color);
  tft.drawLine(BLT_ICON_START_X + BLT_ICON_WIDTH * 1/2, BLT_ICON_START_Y, BLT_ICON_START_X + BLT_ICON_WIDTH * 1/2, BLT_ICON_START_Y + BLT_ICON_HEIGHT, color);
  tft.drawLine(BLT_ICON_START_X + BLT_ICON_WIDTH * 1/2, BLT_ICON_START_Y + BLT_ICON_HEIGHT, BLT_ICON_START_X + BLT_ICON_WIDTH, BLT_ICON_START_Y + BLT_ICON_HEIGHT * 3/4, color);
  tft.drawLine(BLT_ICON_START_X + BLT_ICON_WIDTH, BLT_ICON_START_Y + BLT_ICON_HEIGHT * 3/4, BLT_ICON_START_X, BLT_ICON_START_Y + BLT_ICON_HEIGHT * 1/4, color);
}

void UIManager::playReceiveSignalAnimation() {  // Draw circles for incomming signal
  for (int radius = ICON_OUTER_RADIUS; radius >= ICON_INNER_RADIUS; radius -= ICON_RING_SPACING) {
    tft.drawCircle(SIGNAL_ICON_X, SIGNAL_ICON_Y, radius, ICON_SIGNAL_COLOR);
    delay(FRAME_DELAY);
  }

  for (int radius = ICON_OUTER_RADIUS; radius >= ICON_INNER_RADIUS; radius -= ICON_RING_SPACING) {  
    tft.drawCircle(SIGNAL_ICON_X, SIGNAL_ICON_Y, radius, INVERTED_BG_COLOR);
    delay(FRAME_DELAY);
  }
}

void UIManager::playEmitSignalAnimation() {  // Draw circles for outgoing signal
  for (int radius = ICON_INNER_RADIUS; radius <= ICON_OUTER_RADIUS; radius += ICON_RING_SPACING){    
    tft.drawCircle(SIGNAL_ICON_X, SIGNAL_ICON_Y, radius, ICON_SIGNAL_COLOR);
    delay(FRAME_DELAY);
  }

  for (int radius = ICON_INNER_RADIUS; radius <= ICON_OUTER_RADIUS; radius += ICON_RING_SPACING) {
    tft.drawCircle(SIGNAL_ICON_X, SIGNAL_ICON_Y, radius, DEFAULT_BG_COLOR);
    delay(FRAME_DELAY);
  }
}