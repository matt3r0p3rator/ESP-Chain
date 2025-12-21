#pragma once
#include "module_base.h"
#include <Wire.h>
#include <QMC5883LCompass.h>

class CompassModule : public Module {
public:
    void init() override;
    void loop() override;
    String getName() override;
    String getDescription() override;
    void drawMenu(DisplayManager* display) override;
    bool handleInput(uint8_t button) override;

private:
    QMC5883LCompass compass;
    bool isConnected = false;
    int x = 0, y = 0, z = 0;
    int azimuth = 0;
    
    void drawCompass(DisplayManager* display);
};
