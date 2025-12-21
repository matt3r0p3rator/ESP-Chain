#include "compass_module.h"
#include "display_manager.h"
#include <Arduino.h>
#include <math.h>

#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.017453292519943295769236907684886
#endif

// Default I2C pins for LilyGo T-Display S3 Qwiic/I2C connector
#ifndef SDA_PIN
#define SDA_PIN 43
#endif
#ifndef SCL_PIN
#define SCL_PIN 44
#endif

void CompassModule::init() {
    Serial.println("Initializing Compass...");
    
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000); // Force standard speed
    
    // Check if compass is present at 0x0D
    Wire.beginTransmission(0x0D);
    if (Wire.endTransmission() == 0) {
        compass.init();
        delay(100); // Give it time to start measurements
        isConnected = true;
        Serial.println("Compass found at 0x0D");
    } else {
        isConnected = false;
        Serial.println("Compass NOT found at 0x0D");
    }
}

void CompassModule::loop() {
    if (!isConnected) return;

    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate < 250) {
        yield(); // Feed watchdog
        return; 
    }
    lastUpdate = millis();

    compass.read();
    
    x = compass.getX();
    y = compass.getY();
    z = compass.getZ();
    
    azimuth = compass.getAzimuth();
    
    // Simple redraw request
    extern DisplayManager displayManager;
    drawMenu(&displayManager);
}

String CompassModule::getName() {
    return "Compass";
}

String CompassModule::getDescription() {
    return "QMC5883L Compass";
}

void CompassModule::drawMenu(DisplayManager* display) {
    TFT_eSPI* tft = display->getTFT();
    
    if (!isConnected) {
        display->clearContent();
        display->drawMenuTitle("Compass");
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->drawString("Compass Not Found!", 160, 100, 4);
        tft->drawString("Check Wiring (I2C)", 160, 140, 2);
        tft->drawString("SDA: " + String(SDA_PIN) + " SCL: " + String(SCL_PIN), 160, 170, 2);
        tft->drawString("Long Press to Exit", 160, 220, 2);
        return;
    }

    // Clear only the working area to reduce flicker/load
    tft->fillRect(0, 30, 320, 130, TFT_BLACK);

    drawCompass(display);
}

void CompassModule::drawCompass(DisplayManager* display) {
    TFT_eSPI* tft = display->getTFT();
    char buf[32];
    
    // --- Right Side: Visual Compass ---
    int cx = 240; // Center X for compass
    int cy = 85;  // Center Y for compass
    int r = 45;   // Radius
    
    // Draw Circle
    tft->drawCircle(cx, cy, r, TFT_WHITE);
    tft->drawCircle(cx, cy, r-1, TFT_WHITE);
    
    // Draw N, E, S, W labels
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_RED, TFT_BLACK);
    tft->drawString("N", cx, cy - r - 12, 2);
    
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString("S", cx, cy + r + 12, 2);
    tft->drawString("E", cx + r + 12, cy, 2);
    tft->drawString("W", cx - r - 12, cy, 2);
    
    // Calculate Needle Angle
    float rads = (-azimuth - 90) * DEG_TO_RAD;
    
    int x1 = cx + (r - 8) * cos(rads);
    int y1 = cy + (r - 8) * sin(rads);
    
    int x2 = cx - (r - 8) * cos(rads);
    int y2 = cy - (r - 8) * sin(rads);
    
    // Draw Needle
    tft->drawLine(cx, cy, x1, y1, TFT_RED);   // North Tip
    tft->drawLine(cx, cy, x2, y2, TFT_WHITE); // South Tail
    tft->fillCircle(cx, cy, 3, TFT_DARKGREY);
    
    // --- Left Side: Data ---
    tft->setTextDatum(TL_DATUM);
    int tx = 10;
    int ty = 30;
    int spacing = 20; 
    
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    tft->drawString("Heading:", tx, ty, 2);
    
    snprintf(buf, sizeof(buf), "%d deg", azimuth);
    tft->drawString(buf, tx + 10, ty + spacing, 4);
    
    ty += spacing * 2.5;
    
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString("Raw Data:", tx, ty, 2); ty += spacing;
    
    snprintf(buf, sizeof(buf), "X: %d", x);
    tft->drawString(buf, tx + 10, ty, 2); ty += spacing;
    
    snprintf(buf, sizeof(buf), "Y: %d", y);
    tft->drawString(buf, tx + 10, ty, 2); ty += spacing;
    
    snprintf(buf, sizeof(buf), "Z: %d", z);
    tft->drawString(buf, tx + 10, ty, 2);

    // Footer
    tft->setTextDatum(MC_DATUM);
    tft->drawString("Long Press to Exit", 160, 160, 1);
}

bool CompassModule::handleInput(uint8_t button) {
    if (button == 3) { // Long Press Back
        return false; // Exit
    }
    return true;
}
