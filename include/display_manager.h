#pragma once
#include <TFT_eSPI.h>
#include <RTClib.h>

// Dark Purple Theme Colors
#define THEME_BG        TFT_BLACK
#define THEME_PRIMARY   0x780F // Purple
#define THEME_SECONDARY 0x4010 // Dark Purple
#define THEME_TEXT      TFT_WHITE
#define THEME_ACCENT    0x911F // Light Purple

class DisplayManager {
public:
    DisplayManager();
    void init();
    void setBrightness(int brightness);
    void initRTC();
    void turnOff();
    void clear();
    void clearContent();
    void drawStatusBar(String status, float voltage, bool sdStatus, bool wifiStatus, bool showClock = true, String replacement = "", bool forceRedraw = true);
    void drawMenuTitle(String title);
    void drawMenuItem(String text, int index, bool selected, const unsigned char* icon = nullptr, int iconWidth = 16, int iconHeight = 16, int iconSpacing = 8, int iconOffsetY = 0);
    void updateClock();
    void drawScrollBar(int totalItems, int scrollOffset, int itemsPerPage);
    float getBatteryVoltage();
    bool isOnBattery();
    void setTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
    DateTime getTime();
    TFT_eSPI* getTFT(); // Return pointer to allow null check if needed

private:
    TFT_eSPI* tft;
    RTC_DS3231 rtc;
    bool rtcInitialized;
};
