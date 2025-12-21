#pragma once
#include "module_base.h"
#include <Wire.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include <QMC5883LCompass.h>

class GPSModule : public Module {
public:
    void init() override;
    void loop() override;
    String getName() override;
    String getDescription() override;
    void drawMenu(DisplayManager* display) override;
    bool handleInput(uint8_t button) override;

private:
    SFE_UBLOX_GNSS myGNSS;
    bool isConnected = false;
    long lastUpdate = 0;

    // Compass
    QMC5883LCompass compass;
    bool compassConnected = false;
    int heading = 0;
    
    // GPS Data
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
    uint8_t satellites = 0;
    uint16_t year = 0;
    uint8_t month = 0;
    uint8_t day = 0;
    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;
    bool fix = false;
};
