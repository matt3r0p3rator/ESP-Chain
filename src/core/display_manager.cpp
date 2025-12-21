#include "display_manager.h"
#include "../ui/icons.h"
#include <Wire.h>

#define PIN_BAT_VOLT 4

DisplayManager::DisplayManager() {
    tft = new TFT_eSPI();
    rtcInitialized = false;
}

void DisplayManager::init() {
    // Power on the display (Pin 15)
    pinMode(15, OUTPUT);
    digitalWrite(15, HIGH);
    
    // Battery Pin
    pinMode(PIN_BAT_VOLT, INPUT);

    tft->init();
    tft->setRotation(3); // Landscape
    tft->fillScreen(THEME_BG);
    tft->setTextSize(1);
    tft->setTextColor(THEME_TEXT, THEME_BG);
}

void DisplayManager::initRTC() {
    Wire.begin(43, 44);
    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
        rtcInitialized = false;
    } else {
        rtcInitialized = true;
    }
}

void DisplayManager::turnOff() {
    digitalWrite(15, LOW); // Turn off backlight
    tft->writecommand(TFT_DISPOFF);
    tft->writecommand(TFT_SLPIN);
}

void DisplayManager::clear() {
    tft->fillScreen(THEME_BG);
}

void DisplayManager::clearContent() {
    tft->fillRect(0, 20, 320, 150, THEME_BG);
}
void DisplayManager::drawStatusBar(String status, float voltage, bool sdStatus) {
    tft->fillRect(0, 0, 320, 20, THEME_SECONDARY);
    tft->setTextColor(THEME_TEXT, THEME_SECONDARY);
    tft->setTextDatum(ML_DATUM);
    tft->drawString(status.c_str(), 5, 10, 2);
    
    // Draw Clock
    if (rtcInitialized) {
        DateTime now = rtc.now();
        char timeStr[10];
        sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
        tft->setTextDatum(MC_DATUM);
        tft->drawString(timeStr, 160, 10, 2);
    }

    // Draw SD Icon
    uint16_t color = sdStatus ? TFT_GREEN : TFT_RED;
    tft->drawBitmap(255, 2, image_folder_explorer_bits, 17, 16, color);
    
    tft->setTextDatum(MR_DATUM);
    tft->drawString((String(voltage, 2) + "V").c_str(), 315, 10, 2);
    
    tft->setTextColor(THEME_TEXT, THEME_BG); // Reset
}

float DisplayManager::getBatteryVoltage() {
    uint32_t raw = analogRead(PIN_BAT_VOLT);
    return (raw * 2.0 * 3.3) / 4096.0;
}

void DisplayManager::drawMenuTitle(String title) {
    // Title removed as per user request
}

void DisplayManager::drawMenuItem(String text, int index, bool selected) {
    int yPos = 25 + (index * 25); 
    
    if (selected) {
        tft->fillRect(10, yPos, 300, 22, THEME_PRIMARY);
        tft->setTextColor(THEME_TEXT, THEME_PRIMARY);
    } else {
        tft->fillRect(10, yPos, 300, 22, THEME_BG);
        tft->setTextColor(THEME_TEXT, THEME_BG);
    }
    
    tft->setTextDatum(ML_DATUM);
    tft->drawString(text.c_str(), 20, yPos + 11, 2);
}

void DisplayManager::updateClock() {
    if (rtcInitialized) {
        DateTime now = rtc.now();
        char timeStr[10];
        sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
        
        // Set text color with background to overwrite previous text without flickering
        tft->setTextColor(THEME_TEXT, THEME_SECONDARY);
        tft->setTextDatum(MC_DATUM);
        
        // We might need to clear the area if the font is proportional and the string width changes
        // But for HH:MM:SS with a fixed font or similar digits it's usually fine.
        // To be safe, let's fill a small rect.
        // 160 is center. Text size 2. 
        // Let's just trust the background color overwrite for now, it's usually smoother.
        // Actually, let's use a fixed width padding if needed, but HH:MM:SS is constant length.
        
        // Explicitly clear the clock area to be safe against artifacts
        // tft->fillRect(110, 0, 100, 20, THEME_SECONDARY); 
        // No, fillRect causes flicker.
        
        tft->drawString(timeStr, 160, 10, 2);
        tft->setTextColor(THEME_TEXT, THEME_BG); // Reset
    }
}

void DisplayManager::setTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
    if (rtcInitialized) {
        rtc.adjust(DateTime(year, month, day, hour, minute, second));
    }
}

DateTime DisplayManager::getTime() {
    if (rtcInitialized) {
        return rtc.now();
    }
    return DateTime((uint32_t)0);
}

TFT_eSPI* DisplayManager::getTFT() {
    return tft;
}
