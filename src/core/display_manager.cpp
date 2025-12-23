#include "display_manager.h"
#include "../ui/icons.h"
#include <Wire.h>

#define PIN_BAT_VOLT 4

DisplayManager::DisplayManager() {
    tft = new TFT_eSPI();
    rtcInitialized = false;
}

void DisplayManager::init() {
    // Power on the display/backlight (Pin 15)
    pinMode(15, OUTPUT);
    digitalWrite(15, HIGH);

    // Battery Pin
    pinMode(PIN_BAT_VOLT, INPUT);

    tft->init();
    tft->setRotation(3); // Landscape
    tft->fillScreen(THEME_BG);
    tft->setTextSize(1);
    tft->setTextColor(THEME_TEXT, THEME_BG);
    
    // Setup PWM for backlight (Pin 38 as per TFT_BL, after TFT_eSPI init)
    ledcSetup(0, 5000, 8); // Channel 0, 5kHz, 8-bit resolution
    ledcAttachPin(38, 0);
    ledcWrite(0, 128); // Default brightness
}

void DisplayManager::setBrightness(int brightness) {
    ledcWrite(0, brightness);
}

void DisplayManager::initRTC() {
    // Enable external power (Pin 17) for I2C devices
    pinMode(17, OUTPUT);
    digitalWrite(17, HIGH);
    delay(100);

    Wire.begin(43, 44);
    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
        rtcInitialized = false;
    } else {
        rtcInitialized = true;
    }
}

void DisplayManager::turnOff() {
    setBrightness(0); // Turn off backlight
    tft->writecommand(TFT_DISPOFF);
    tft->writecommand(TFT_SLPIN);
}

void DisplayManager::clear() {
    tft->fillScreen(THEME_BG);
}

void DisplayManager::clearContent() {
    tft->fillRect(0, 20, 320, 150, THEME_BG);
}

void DisplayManager::drawStatusBar(String status, float voltage, bool sdStatus, bool wifiStatus, bool showClock, String replacement) {
    tft->fillRect(0, 0, 320, 20, THEME_SECONDARY);
    tft->setTextColor(THEME_TEXT, THEME_SECONDARY);
    tft->setTextDatum(ML_DATUM);
    tft->drawString(status.c_str(), 5, 10, 2);
    
    // Draw Clock or Replacement
    tft->setTextDatum(MC_DATUM);
    if (showClock) {
        if (rtcInitialized) {
            DateTime now = rtc.now();
            char timeStr[10];
            sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
            tft->drawString(timeStr, 160, 10, 2);
        }
    } else {
        tft->drawString(replacement.c_str(), 160, 10, 2);
    }

    // Draw WiFi Icon
    if (wifiStatus) {
        tft->setTextColor(TFT_CYAN, THEME_SECONDARY);
        tft->drawBitmap(235, 2, image_cloud_sync_bits, 17, 16, TFT_CYAN);
    }

    // Draw SD Icon
    uint16_t color = sdStatus ? TFT_CYAN : TFT_RED; // White
    tft->drawBitmap(255, 2, sdStatus ? image_micro_sd_bits : image_micro_sd_no_card_bits, 14, 16, color);
    
    tft->setTextDatum(MR_DATUM);
    tft->setTextColor(THEME_TEXT, THEME_SECONDARY);
    tft->drawString((String(voltage, 2) + "V").c_str(), 315, 10, 2);
    
    tft->setTextColor(THEME_TEXT, THEME_BG); // Reset
}

float DisplayManager::getBatteryVoltage() {
    uint32_t raw = analogRead(PIN_BAT_VOLT);
    return (raw * 2.0 * 3.3) / 4096.0;
}

bool DisplayManager::isOnBattery() {
    float voltage = getBatteryVoltage();
    return voltage < 4.0;  // Adjust threshold if needed (e.g., < 4.5 for USB detection)
}

void DisplayManager::drawMenuTitle(String title) {
    // Title removed as per user request
}

void DisplayManager::drawMenuItem(String text, int index, bool selected, const unsigned char* icon, int iconWidth, int iconHeight, int iconSpacing, int iconOffsetY) {
    int yPos = 25 + (index * 25); 
    int radius = 4;
    int width = 290;
    
    if (selected) {
        tft->fillRoundRect(10, yPos, width, 22, radius, THEME_PRIMARY);
        tft->setTextColor(THEME_TEXT, THEME_PRIMARY);
    } else {
        tft->fillRoundRect(10, yPos, width, 22, radius, THEME_BG);
        tft->setTextColor(THEME_TEXT, THEME_BG);
    }
    tft->drawRoundRect(10, yPos, width, 22, radius, TFT_WHITE);
    
    int textX = 20;
    if (icon) {
        int iconY = yPos + (22 - iconHeight) / 2 + iconOffsetY;
        tft->drawBitmap(textX, iconY, icon, iconWidth, iconHeight, THEME_TEXT);
        textX += iconWidth + iconSpacing;
    }
    
    tft->setTextDatum(ML_DATUM);
    tft->drawString(text.c_str(), textX, yPos + 11, 2);
}

void DisplayManager::drawScrollBar(int totalItems, int currentItem, int visibleItems) {
    if (totalItems <= visibleItems) return;
    
    int scrollBarX = 308;
    int scrollBarY = 25;
    int scrollBarWidth = 6;
    int scrollBarHeight = 125; // 5 items * 25px
    // Draw track
    tft->drawRoundRect(scrollBarX, scrollBarY, scrollBarWidth, scrollBarHeight, 3, THEME_SECONDARY);
    // Calculate thumb
    float ratio = (float)visibleItems / totalItems;
    int thumbHeight = scrollBarHeight * ratio;
    if (thumbHeight < 10) thumbHeight = 10;
    // Calculate thumb position
    int maxScroll = totalItems - visibleItems;
    float scrollRatio = (float)currentItem / maxScroll;
    int maxThumbY = scrollBarHeight - thumbHeight;
    int thumbY = scrollBarY + (scrollRatio * maxThumbY);
    tft->fillRoundRect(scrollBarX + 1, thumbY + 1, scrollBarWidth - 2, thumbHeight - 2, 2, TFT_WHITE);
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
