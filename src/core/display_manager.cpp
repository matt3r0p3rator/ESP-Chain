#include "display_manager.h"
#include "../ui/icons.h"
#include <Wire.h>
#include <WiFi.h>
#include <SD.h>
#include <SPI.h>

#define PIN_BAT_VOLT 4
#define PIN_SD_CS 10

DisplayManager::DisplayManager() {
    tft = new TFT_eSPI();
    statusSprite = nullptr;
    menuSprite = nullptr;
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
    
    // Initialize Sprites
    statusSprite = new TFT_eSprite(tft);
    statusSprite->createSprite(320, 20);
    
    menuSprite = new TFT_eSprite(tft);
    menuSprite->createSprite(320, 150);

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
    delay(1000);

    Wire.begin(43, 44);
    
    for (int i = 0; i < 3; i++) {
        if (rtc.begin()) {
            rtcInitialized = true;
            break;
        }
        Serial.println("Couldn't find RTC, retrying...");
        delay(500);
    }

    if (!rtcInitialized) {
        Serial.println("Couldn't find RTC");
    }
}

void DisplayManager::turnOff() {
    // Fade out effect - gradually reduce brightness
    for (int brightness = 128; brightness >= 0; brightness -= 4) {
        setBrightness(brightness);
        delay(30); // Small delay for smooth fade effect
    }
    
    setBrightness(0); // Ensure fully off
    tft->writecommand(TFT_DISPOFF);
    tft->writecommand(TFT_SLPIN);
}

void DisplayManager::clear() {
    tft->fillScreen(THEME_BG);
}

void DisplayManager::clearContent() {
    tft->fillRect(0, 20, 320, 150, THEME_BG);
}

void DisplayManager::clearMenu() {
    if (menuSprite) {
        menuSprite->fillSprite(THEME_BG);
    }
}

void DisplayManager::updateMenu() {
    if (menuSprite) {
        menuSprite->pushSprite(0, 20);
    }
}

void DisplayManager::drawStatusBar(String status, float voltage, bool sdStatus, bool wifiStatus, bool showClock, String replacement, bool forceRedraw) {
    if (!statusSprite) return;

    statusSprite->fillSprite(THEME_SECONDARY);

    statusSprite->setTextColor(THEME_TEXT, THEME_SECONDARY);
    statusSprite->setTextDatum(ML_DATUM);
    statusSprite->setTextPadding(100);
    statusSprite->drawString(status.c_str(), 5, 10, 2);
    statusSprite->setTextPadding(0);
    
    // Draw Clock or Replacement
    statusSprite->setTextDatum(MC_DATUM);
    statusSprite->setTextPadding(100);
    if (showClock) {
        if (rtcInitialized) {
            DateTime now = rtc.now();
            char timeStr[10];
            sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
            statusSprite->drawString(timeStr, 160, 10, 2);
        }
    } else {
        statusSprite->drawString(replacement.c_str(), 160, 10, 2);
    }
    statusSprite->setTextPadding(0);

    // Draw Voltage Text (First to avoid padding erasing icons)
    statusSprite->setTextDatum(MR_DATUM);
    statusSprite->setTextPadding(40);
    statusSprite->setTextColor(THEME_TEXT, THEME_SECONDARY);
    statusSprite->drawString((String(voltage, 2) + "V").c_str(), 320, 10, 2);
    statusSprite->setTextPadding(0);

    // Draw WiFi Icon
    // statusSprite->fillRect(215, 2, 18, 16, THEME_SECONDARY); // Not needed with fillSprite
    if (wifiStatus) {
        statusSprite->setTextColor(TFT_CYAN, THEME_SECONDARY);
        statusSprite->drawBitmap(215, 2, image_cloud_sync_bits, 17, 16, TFT_CYAN);
    }

    // Draw SD Icon
    // statusSprite->fillRect(235, 2, 15, 16, THEME_SECONDARY);
    uint16_t color = sdStatus ? TFT_CYAN : TFT_RED; // White
    statusSprite->drawBitmap(235, 2, sdStatus ? image_micro_sd_bits : image_micro_sd_no_card_bits, 14, 16, color);
    
    // Draw Battery Icon
    // statusSprite->fillRect(255, 2, 25, 16, THEME_SECONDARY);
    const unsigned char* batIcon = image_battery_full_bits;
    
    if (voltage >= 4) {
        batIcon = image_battery_charging_bits;
    } else {
        float percentage = (voltage - 3.3) / (3.8 - 3.4);
        if (percentage < 0) percentage = 0;
        if (percentage > 1) percentage = 1;
        
        if (percentage < 0.10) batIcon = image_battery_0_bits;
        else if (percentage < 0.17) batIcon = image_battery_17_bits;
        else if (percentage < 0.33) batIcon = image_battery_33_bits;
        else if (percentage < 0.50) batIcon = image_battery_50_bits;
        else if (percentage < 0.67) batIcon = image_battery_67_bits;
        else if (percentage < 0.83) batIcon = image_battery_83_bits;
        else batIcon = image_battery_full_bits;
    }
    
    statusSprite->drawBitmap(255, 2, batIcon, 24, 16, THEME_TEXT);
    
    statusSprite->pushSprite(0, 0);
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
    // Set the title text on the top bar
    float voltage = getBatteryVoltage();
    bool sdStatus = SD.begin(PIN_SD_CS);
    bool wifiStatus = (WiFi.status() == WL_CONNECTED);
    drawStatusBar(title, voltage, sdStatus, wifiStatus);
}

void DisplayManager::drawMenuItem(String text, int index, bool selected, const unsigned char* icon, int iconWidth, int iconHeight, int iconSpacing, int iconOffsetY) {
    if (!menuSprite) return;

    // Adjust Y position for sprite (relative to 0,0 of sprite, which is 0,20 on screen)
    // Original: 25 + (index * 25)
    // New: 5 + (index * 25)
    int yPos = 5 + (index * 25); 
    int radius = 4;
    int width = 290;
    
    if (selected) {
        menuSprite->fillRoundRect(10, yPos, width, 22, radius, THEME_PRIMARY);
        menuSprite->setTextColor(THEME_TEXT, THEME_PRIMARY);
    } else {
        menuSprite->fillRoundRect(10, yPos, width, 22, radius, THEME_BG);
        menuSprite->setTextColor(THEME_TEXT, THEME_BG);
    }
    menuSprite->drawRoundRect(10, yPos, width, 22, radius, TFT_WHITE);
    
    int textX = 20;
    if (icon) {
        int iconY = yPos + (22 - iconHeight) / 2 + iconOffsetY;
        menuSprite->drawBitmap(textX, iconY, icon, iconWidth, iconHeight, THEME_TEXT);
        textX += iconWidth + iconSpacing;
    }
    
    menuSprite->setTextDatum(ML_DATUM);
    menuSprite->drawString(text.c_str(), textX, yPos + 11, 2);
}

void DisplayManager::drawScrollBar(int totalItems, int currentItem, int visibleItems) {
    if (!menuSprite) return;
    if (totalItems <= visibleItems) return;
    
    int scrollBarX = 308;
    int scrollBarY = 5; // Adjusted for sprite (was 25)
    int scrollBarWidth = 6;
    int scrollBarHeight = 125; // 5 items * 25px
    // Draw track
    menuSprite->drawRoundRect(scrollBarX, scrollBarY, scrollBarWidth, scrollBarHeight, 3, THEME_SECONDARY);
    // Calculate thumb
    float ratio = (float)visibleItems / totalItems;
    int thumbHeight = scrollBarHeight * ratio;
    if (thumbHeight < 10) thumbHeight = 10;
    // Calculate thumb position
    int maxScroll = totalItems - visibleItems;
    float scrollRatio = (float)currentItem / maxScroll;
    int maxThumbY = scrollBarHeight - thumbHeight;
    int thumbY = scrollBarY + (scrollRatio * maxThumbY);
    menuSprite->fillRoundRect(scrollBarX + 1, thumbY + 1, scrollBarWidth - 2, thumbHeight - 2, 2, TFT_WHITE);
}

void DisplayManager::updateClock() {
    // Deprecated/Unused in favor of drawStatusBar
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