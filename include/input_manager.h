#pragma once
#include <Arduino.h>
#include <ESP32Encoder.h>
#include "menu_system.h"

// Pin Definitions
#define ENCODER_CLK 18
#define ENCODER_DT  17
#define ENCODER_SW  21
#define BTN_BOTTOM  14

class InputManager {
public:
    InputManager(MenuSystem* menuSystem);
    void begin();
    void update();

private:
    MenuSystem* menuSystem;
    ESP32Encoder encoder;
    
    long lastEncoderPosition = 0;
    
    // Encoder Button State
    int lastBtnState = HIGH;
    unsigned long btnPressTime = 0;
    bool btnLongPressHandled = false;

    // Bottom Button State
    int lastBottomBtnState = HIGH;
    unsigned long bottomBtnPressTime = 0;
    
    const unsigned long DEBOUNCE_DELAY = 50;
    const unsigned long LONG_PRESS_DELAY = 500;
};
