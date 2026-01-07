#include "input_manager.h"

InputManager::InputManager(MenuSystem* menu) : menuSystem(menu) {}

void InputManager::begin() {
    pinMode(BTN_0, INPUT_PULLUP);
    pinMode(BTN_14, INPUT_PULLUP);
}

void InputManager::update() {
    unsigned long currentMillis = millis();

    // --- Button 0 Handling ---
    int reading0 = digitalRead(BTN_0);

    if (reading0 != lastBtn0State) {
        // State changed
        if (reading0 == LOW) { // Pressed
            btn0PressTime = currentMillis;
            btn0LongPressHandled = false;
        } else { // Released
            if (!btn0LongPressHandled) {
                 // Single Click Action: Scroll Up (0)
                 menuSystem->handleInput(0);
            }
        }
        delay(DEBOUNCE_DELAY); // Simple debounce
    } else if (reading0 == LOW) {
        // Held down
        if (!btn0LongPressHandled && (currentMillis - btn0PressTime > LONG_PRESS_DELAY)) {
            // Long Press: Select (2)
            menuSystem->handleInput(2);
            btn0LongPressHandled = true;
        }
    }
    lastBtn0State = reading0;

    // --- Button 14 Handling ---
    int reading14 = digitalRead(BTN_14);

    if (reading14 != lastBtn14State) {
        // State changed
        if (reading14 == LOW) { // Pressed
            btn14PressTime = currentMillis;
            btn14LongPressHandled = false;
        } else { // Released
            if (!btn14LongPressHandled) {
                // Single Click Action: Scroll Down (1)
                menuSystem->handleInput(1);
            }
        }
        delay(DEBOUNCE_DELAY); // Simple debounce
    } else if (reading14 == LOW) {
        // Held down
        if (!btn14LongPressHandled && (currentMillis - btn14PressTime > LONG_PRESS_DELAY)) {
            // Long Press: Back (3)
            menuSystem->handleInput(3);
            btn14LongPressHandled = true;
        }
    }
    
    lastBtn14State = reading14;
}
