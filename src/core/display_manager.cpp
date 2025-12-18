#include "display_manager.h"

DisplayManager::DisplayManager() {
    tft = new TFT_eSPI();
}

void DisplayManager::init() {
    Serial.println("DisplayManager::init start");
    if (tft) {
        tft->init();
        Serial.println("tft.init done");
        tft->setRotation(3); // Landscape
        Serial.println("tft.setRotation done");
        tft->fillScreen(TFT_BLACK);
        Serial.println("tft.fillScreen done");
        tft->setTextColor(TFT_GREEN, TFT_BLACK);
        tft->setTextSize(2);
    }
    Serial.println("DisplayManager::init end");
}

void DisplayManager::clear() {
    if (tft) tft->fillScreen(TFT_BLACK);
}

void DisplayManager::drawStatusBar(String status, int batteryLevel) {
    if (!tft) return;
    tft->fillRect(0, 0, 320, 20, TFT_DARKGREY);
    tft->setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft->setTextSize(1);
    tft->setCursor(5, 5);
    tft->print(status);
    tft->setCursor(250, 5);
    tft->print("Bat: " + String(batteryLevel) + "%");
}

void DisplayManager::drawMenuTitle(String title) {
    if (!tft) return;
    tft->setTextColor(TFT_GREEN, TFT_BLACK);
    tft->setTextSize(2);
    tft->setCursor(10, 30);
    tft->println(title);
    tft->drawLine(0, 50, 320, 50, TFT_GREEN);
}

TFT_eSPI& DisplayManager::getTFT() {
    return *tft;
}
