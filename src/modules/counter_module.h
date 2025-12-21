#pragma once
#include <Arduino.h>
#include "module_base.h"
#include "display_manager.h"

class CounterModule : public Module {
private:
    int counter = 0;

public:
    void init() override {
        // Reset counter when entering module? Or keep state?
        // Let's keep state to show persistence while running
    }

    void loop() override {
        // Nothing to do in loop
    }

    String getName() override {
        return "Counter Demo";
    }

    String getDescription() override {
        return "Click to Count";
    }

    void drawMenu(DisplayManager* display) override {
        display->clearContent();
        display->drawMenuTitle("Counter");
        
        display->getTFT()->setTextDatum(MC_DATUM);
        display->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
        
        // Draw the big number
        display->getTFT()->setTextSize(2); // Double current size
        display->getTFT()->drawString(String(counter), 160, 90, 4);
        display->getTFT()->setTextSize(1); // Reset size
        
        display->getTFT()->drawString("Single Click: +1", 160, 140, 2);
        display->getTFT()->drawString("Double Click: -1", 160, 160, 2);
    }

    bool handleInput(uint8_t button) override {
        extern DisplayManager displayManager;
        
        if (button == 3) return false; // Long Press -> Exit
        
        if (button == 1) { // Single Click
            counter++;
            drawMenu(&displayManager);
            return true;
        }
        
        if (button == 2) { // Double Click
            counter--;
            drawMenu(&displayManager);
            return true;
        }
        
        return true;
    }
};
