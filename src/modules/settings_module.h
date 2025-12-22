#pragma once
#include <Arduino.h>
#include "module_base.h"
#include "config_manager.h"
#include "display_manager.h"

class SettingsModule : public Module {
private:
    enum State { STATE_MAIN, STATE_DISPLAY, STATE_WIFI, STATE_BADUSB, STATE_TIME };
    State currentState;
    int menuIndex;
    int editHour;
    int editMinute;
    
    String getBoolStr(bool val) { 
        return val ? "ON" : "OFF"; 
    }

public:
    void init() override {
        currentState = STATE_MAIN;
        menuIndex = 0;
        ConfigManager::getInstance().load();
    }

    void loop() override {}

    String getName() override { 
        return "Settings"; 
    }
    const unsigned char* getIcon() override { return image_menu_options_bits; }
    int getIconWidth() override { return 14; }
    int getIconHeight() override { return 16; }
    int getIconOffsetY() override { return 1; }
    
    String getDescription() override { 
        return "Edit Config"; 
    }

    void drawMenu(DisplayManager* display) override {
        display->clearContent();
        ConfigData& data = ConfigManager::getInstance().data;

        if (currentState == STATE_MAIN) {
            const char* items[] = {"Display", "WiFi", "BadUSB", "Time", "Save & Exit"};
            for (int i = 0; i < 5; i++) {
                display->drawMenuItem(items[i], i, i == menuIndex);
            }
            display->drawScrollBar(5, 0, 5);
        }
        else if (currentState == STATE_DISPLAY) {
            display->drawMenuItem("Bright: " + String(data.displayBrightness), 0, menuIndex == 0);
            String toStr = (data.displayTimeout == -1) ? "Always On" : String(data.displayTimeout) + "s";
            display->drawMenuItem("Timeout: " + toStr, 1, menuIndex == 1);
            display->drawMenuItem("Back", 2, menuIndex == 2);
            display->drawScrollBar(3, 0, 5);
        }
        else if (currentState == STATE_WIFI) {
            display->drawMenuItem("AutoScan: " + getBoolStr(data.wifiAutoScan), 0, menuIndex == 0);
            display->drawMenuItem("SaveHS: " + getBoolStr(data.wifiSaveHandshakes), 1, menuIndex == 1);
            display->drawMenuItem("Reason: " + String(data.wifiDeauthReason), 2, menuIndex == 2);
            display->drawMenuItem("Back", 3, menuIndex == 3);
            display->drawScrollBar(4, 0, 5);
        }
        else if (currentState == STATE_BADUSB) {
            display->drawMenuItem("Def Dly: " + String(data.badusbDelay) + "ms", 0, menuIndex == 0);
            display->drawMenuItem("Start Dly: " + String(data.badusbStartupDelay) + "ms", 1, menuIndex == 1);
            display->drawMenuItem("AutoExec: " + getBoolStr(data.badusbAutoExec), 2, menuIndex == 2);
            display->drawMenuItem("Back", 3, menuIndex == 3);
            display->drawScrollBar(4, 0, 5);
        }
        else if (currentState == STATE_TIME) {
            display->drawMenuItem("Hour: " + String(editHour), 0, menuIndex == 0);
            display->drawMenuItem("Minute: " + String(editMinute), 1, menuIndex == 1);
            display->drawMenuItem("Save", 2, menuIndex == 2);
            display->drawMenuItem("Back", 3, menuIndex == 3);
            display->drawScrollBar(4, 0, 5);
        }
    }

    bool handleInput(uint8_t button) override {
        extern DisplayManager displayManager;
        ConfigData& data = ConfigManager::getInstance().data;

        if (button == 1) { // Scroll
            int maxItems = 0;
            if (currentState == STATE_MAIN) maxItems = 5;
            else if (currentState == STATE_DISPLAY) maxItems = 3;
            else if (currentState == STATE_WIFI) maxItems = 4;
            else if (currentState == STATE_BADUSB) maxItems = 4;
            else if (currentState == STATE_TIME) maxItems = 4;
            
            menuIndex = (menuIndex + 1) % maxItems;
            drawMenu(&displayManager);
            return true;
        }

        if (button == 2) { // Select / Toggle / Edit
            if (currentState == STATE_MAIN) {
                if (menuIndex == 0) { currentState = STATE_DISPLAY; menuIndex = 0; }
                else if (menuIndex == 1) { currentState = STATE_WIFI; menuIndex = 0; }
                else if (menuIndex == 2) { currentState = STATE_BADUSB; menuIndex = 0; }
                else if (menuIndex == 3) { 
                    currentState = STATE_TIME; 
                    menuIndex = 0;
                    DateTime now = displayManager.getTime();
                    editHour = now.hour();
                    editMinute = now.minute();
                }
                else if (menuIndex == 4) { 
                    ConfigManager::getInstance().save(); 
                    return false; // Exit module
                }
            }
            else if (currentState == STATE_DISPLAY) {
                if (menuIndex == 0) { // Brightness
                    data.displayBrightness = (data.displayBrightness + 32);
                    if (data.displayBrightness > 255) data.displayBrightness = 32;
                }
                else if (menuIndex == 1) { // Timeout
                    if (data.displayTimeout == -1) data.displayTimeout = 10;
                    else {
                        data.displayTimeout += 10;
                        if (data.displayTimeout > 60) data.displayTimeout = -1;
                    }
                }
                else if (menuIndex == 2) { currentState = STATE_MAIN; menuIndex = 0; }
            }
            else if (currentState == STATE_WIFI) {
                if (menuIndex == 0) data.wifiAutoScan = !data.wifiAutoScan;
                else if (menuIndex == 1) data.wifiSaveHandshakes = !data.wifiSaveHandshakes;
                else if (menuIndex == 2) {
                    data.wifiDeauthReason++;
                    if (data.wifiDeauthReason > 20) data.wifiDeauthReason = 1;
                }
                else if (menuIndex == 3) { currentState = STATE_MAIN; menuIndex = 0; }
            }
            else if (currentState == STATE_BADUSB) {
                if (menuIndex == 0) {
                    data.badusbDelay += 100;
                    if (data.badusbDelay > 1000) data.badusbDelay = 0;
                }
                else if (menuIndex == 1) {
                    data.badusbStartupDelay += 500;
                    if (data.badusbStartupDelay > 10000) data.badusbStartupDelay = 0;
                }
                else if (menuIndex == 2) data.badusbAutoExec = !data.badusbAutoExec;
                else if (menuIndex == 3) { currentState = STATE_MAIN; menuIndex = 0; }
            }
            else if (currentState == STATE_TIME) {
                if (menuIndex == 0) { // Hour
                    editHour = (editHour + 1) % 24;
                }
                else if (menuIndex == 1) { // Minute
                    editMinute = (editMinute + 1) % 60;
                }
                else if (menuIndex == 2) { // Save
                    DateTime now = displayManager.getTime();
                    // Preserve date, update time
                    displayManager.setTime(now.year(), now.month(), now.day(), editHour, editMinute, 0);
                    currentState = STATE_MAIN;
                    menuIndex = 3;
                }
                else if (menuIndex == 3) { // Back
                    currentState = STATE_MAIN;
                    menuIndex = 3;
                }
            }
            drawMenu(&displayManager);
            return true;
        }
        
        if (button == 3) { // Back
             if (currentState != STATE_MAIN) {
                 currentState = STATE_MAIN;
                 menuIndex = 0;
                 drawMenu(&displayManager);
                 return true;
             }
             return false; // Exit
        }

        return true;
    }
};
