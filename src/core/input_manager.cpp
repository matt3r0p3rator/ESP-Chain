#include "input_manager.h"

InputManager::InputManager(MenuSystem* menuSystem) : menuSystem(menuSystem) {}

void InputManager::begin() {
    // Init Encoder
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    encoder.attachHalfQuad(ENCODER_DT, ENCODER_CLK);
    encoder.setCount(0);
    encoder.setFilter(1023); // Add filter for debouncing

    pinMode(ENCODER_SW, INPUT_PULLUP);
    pinMode(BTN_BOTTOM, INPUT_PULLUP);
}

void InputManager::update() {
    // Handle Encoder Rotation
    long newPosition = encoder.getCount() / 2;
    if (newPosition != lastEncoderPosition) {
        if (newPosition > lastEncoderPosition) {
            menuSystem->handleInput(1); // Down
        } else {
            menuSystem->handleInput(0); // Up
        }
        lastEncoderPosition = newPosition;
    }

    // Handle Encoder Button
    int btnState = digitalRead(ENCODER_SW);
    
    if (btnState == LOW && lastBtnState == HIGH) {
        // Button Pressed
        btnPressTime = millis();
        btnLongPressHandled = false;
    } else if (btnState == LOW && lastBtnState == LOW) {
        // Button Held
        if (!btnLongPressHandled && (millis() - btnPressTime > LONG_PRESS_DELAY)) {
            menuSystem->handleInput(3); // Back (Long Press)
            btnLongPressHandled = true;
        }
    } else if (btnState == HIGH && lastBtnState == LOW) {
        // Button Released
        if (!btnLongPressHandled) {
            // Short Press
            if (millis() - btnPressTime > DEBOUNCE_DELAY) {
                menuSystem->handleInput(2); // Select
            }
        }
    }
    lastBtnState = btnState;

    // Handle Bottom Button (Simple Press)
    int bottomBtnState = digitalRead(BTN_BOTTOM);
    if (bottomBtnState == LOW && lastBottomBtnState == HIGH) {
        bottomBtnPressTime = millis();
    } else if (bottomBtnState == HIGH && lastBottomBtnState == LOW) {
        if (millis() - bottomBtnPressTime > DEBOUNCE_DELAY) {
            menuSystem->handleInput(3); // Back
        }
    }
    lastBottomBtnState = bottomBtnState;
}
