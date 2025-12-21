#pragma once
#include <Arduino.h>
#include "menu_system.h"

// Pin Definitions
#define BTN_0  0
#define BTN_14 14

class InputManager {
public:
    InputManager(MenuSystem* menuSystem);
    void begin();
    void update();

private:
    MenuSystem* menuSystem;
    
    // Button 0 State
    int lastBtn0State = HIGH;
    unsigned long btn0PressTime = 0;
    bool btn0LongPressHandled = false;

    // Button 14 State
    int lastBtn14State = HIGH;
    unsigned long btn14PressTime = 0;
    bool btn14LongPressHandled = false;
    
    // Double Click Logic
    int clickCount = 0;
    unsigned long lastClickTime = 0;
    
    const unsigned long DEBOUNCE_DELAY = 50;
    const unsigned long LONG_PRESS_DELAY = 500;
    const unsigned long DOUBLE_CLICK_DELAY = 300; // Max time between clicks
};
