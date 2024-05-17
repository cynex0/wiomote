#include <TFT_eSPI.h>

// Wifi connection icon
#define WIFI_ICON_CIRCLE_X                290
#define WIFI_ICON_CIRCLE_Y                 35
#define WIFI_ICON_CIRCLE_MAX_RAD           21
#define WIFI_ICON_CIRCLE_RADIUS_DIFF        7
#define WIFI_ICON_ICON_COLOR_ON      TFT_CYAN
#define WIFI_ICON_ICON_COLOR_OFF TFT_DARKGREY

// Bluetooth connection icon
#define BLT_ICON_START_X            245
#define BLT_ICON_START_Y             11
#define BLT_ICON_WIDTH               15
#define BLT_ICON_HEIGHT              25
#define BLT_ICON_COLOR_ON      TFT_CYAN
#define BLT_ICON_COLOR_OFF TFT_DARKGREY


// TODO:
// !IMPORTANT These macros should be received from another file later
#define CONNECTED    0 
#define CONNECTING   1
#define DISCONNECTED 2



void drawWiFiConnectionIcon(TFT_eSPI, bool);

void drawBltConnectionIcon(TFT_eSPI, bool);