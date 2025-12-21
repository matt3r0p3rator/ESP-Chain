#pragma once
#include <Arduino.h>
#include "USBHIDKeyboard.h"

class DuckyParser {
public:
    DuckyParser(USBHIDKeyboard* keyboard);
    void parseFile(String filePath);
    void processLine(String line);
    
private:
    USBHIDKeyboard* _keyboard;
    int defaultDelay = 0;
    
    void pressKey(String key);
    void pressCombination(String modifiers, String key);
    uint8_t getKeyCode(String key);
    uint8_t getModifier(String key);
};
