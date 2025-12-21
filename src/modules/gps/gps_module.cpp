#include "gps_module.h"
#include "display_manager.h"

// Default I2C pins for LilyGo T-Display S3 Qwiic/I2C connector
#ifndef SDA_PIN
#define SDA_PIN 43
#endif
#ifndef SCL_PIN
#define SCL_PIN 44
#endif

// GPS Pins
#ifndef GPS_RX_PIN
#define GPS_RX_PIN 2 // Connect GPS TX to this Pin
#endif
#ifndef GPS_TX_PIN
#define GPS_TX_PIN 1 // Connect GPS RX to this Pin
#endif

void GPSModule::init() {
    extern DisplayManager displayManager;
    TFT_eSPI* tft = displayManager.getTFT();
    
    displayManager.clearContent();
    displayManager.drawMenuTitle("GPS Module");
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString("Initializing...", 160, 100, 4);

    // --- Initialize GPS (UART) ---
    Serial.println("Initializing GPS on UART...");
    
    // Give GPS some time to boot
    delay(1500);

    const uint32_t baudRates[] = {115200, 38400, 9600};
    const int numBauds = 3;
    
    // Try to connect multiple times
    for (int attempt = 0; attempt < 3; attempt++) {
        if (isConnected) break;
        
        Serial.printf("GPS Connection Attempt %d/3\n", attempt + 1);
        tft->fillRect(0, 130, 320, 40, TFT_BLACK);
        tft->drawString("Attempt " + String(attempt + 1) + "/3", 160, 150, 2);
        
        for (int i = 0; i < numBauds; i++) {
            Serial1.begin(baudRates[i], SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
            delay(100); // Allow UART to stabilize
            
            if (myGNSS.begin(Serial1)) {
                Serial.printf("u-blox GNSS connected at %d baud\n", baudRates[i]);
                isConnected = true;
                break;
            }
            
            Serial1.end();
            delay(50);
        }
        
        if (!isConnected) delay(500);
    }

    if (isConnected) {
        tft->drawString("GPS Connected!", 160, 180, 2);
        delay(500);
        myGNSS.setUART1Output(COM_TYPE_UBX); // Set the UART1 port to output UBX only
        myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT);
    } else {
        tft->drawString("GPS Not Found", 160, 180, 2);
        delay(1000);
        Serial.println("u-blox GNSS not found on UART after retries");
    }

    // --- Initialize Compass (I2C) ---
    Wire.begin(SDA_PIN, SCL_PIN);
    
    // Check if compass is present at 0x0D
    Wire.beginTransmission(0x0D);
    if (Wire.endTransmission() == 0) {
        compass.init();
        compassConnected = true;
        Serial.println("Compass found at 0x0D");
    } else {
        compassConnected = false;
        Serial.println("Compass NOT found at 0x0D");
    }
}

void GPSModule::loop() {
    // Read Compass
    if (compassConnected) {
        compass.read();
        heading = compass.getAzimuth();
    }

    if (!isConnected) return;

    // Update GPS every 1 second
    if (millis() - lastUpdate > 1000) {
        lastUpdate = millis();
        
        latitude = myGNSS.getLatitude() / 10000000.0;
        longitude = myGNSS.getLongitude() / 10000000.0;
        altitude = myGNSS.getAltitude() / 1000.0; // meters
        satellites = myGNSS.getSIV();
        fix = myGNSS.getGnssFixOk();
        
        year = myGNSS.getYear();
        month = myGNSS.getMonth();
        day = myGNSS.getDay();
        hour = myGNSS.getHour();
        minute = myGNSS.getMinute();
        second = myGNSS.getSecond();
        
        // Force Redraw
        extern DisplayManager displayManager;
        drawMenu(&displayManager);
    }
}

String GPSModule::getName() {
    return "GPS";
}

String GPSModule::getDescription() {
    return "HGLRC M100-5883";
}

void GPSModule::drawMenu(DisplayManager* display) {
    display->clearContent();
    display->drawMenuTitle("GPS Module");
    
    TFT_eSPI* tft = display->getTFT();
    tft->setTextDatum(ML_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    
    if (!isConnected) {
        tft->drawString("GPS Not Found!", 20, 60, 4);
        tft->drawString("Check Wiring", 20, 100, 2);
        tft->drawString("TX: " + String(GPS_TX_PIN) + " RX: " + String(GPS_RX_PIN), 20, 130, 2);
    } else {
        int y = 50;
        int spacing = 25;
        
        String fixStr = fix ? "Fix: YES" : "Fix: NO";
        tft->drawString(fixStr, 20, y, 2); y += spacing;
        
        tft->drawString("Sats: " + String(satellites), 20, y, 2); y += spacing;
        
        tft->drawString("Lat: " + String(latitude, 6), 20, y, 2); y += spacing;
        tft->drawString("Lon: " + String(longitude, 6), 20, y, 2); y += spacing;
        tft->drawString("Alt: " + String(altitude, 1) + "m", 20, y, 2); y += spacing;
        
        if (compassConnected) {
            tft->drawString("Head: " + String(heading), 20, y, 2); y += spacing;
        }
        
        char timeStr[20];
        sprintf(timeStr, "%02d:%02d:%02d UTC", hour, minute, second);
        tft->drawString(timeStr, 20, y, 2);
    }
    
    tft->setTextDatum(MC_DATUM);
    tft->drawString("Long Press to Exit", 160, 220, 2);
}

bool GPSModule::handleInput(uint8_t button) {
    if (button == 3) { // Long Press Back
        return false; // Exit
    }
    return true;
}
