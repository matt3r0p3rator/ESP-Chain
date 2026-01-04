#pragma once
#include <Arduino.h>
#include "module_base.h"
#include "config_manager.h"
#include "display_manager.h"

class SettingsModule : public Module {
private:
    enum State { STATE_MAIN, STATE_DISPLAY, STATE_WIFI, STATE_BADUSB, STATE_TIME, STATE_SECURITY, STATE_SECURITY_PIN };
    State currentState;
    int menuIndex;
    int editHour;
    int editMinute;
    char tempPin[5];
    
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
        display->clearMenu();
        ConfigData& data = ConfigManager::getInstance().data;

        if (currentState == STATE_MAIN) {
            const char* items[] = {"Display", "WiFi", "BadUSB", "Time", "Security", "Save & Exit"};
            for (int i = 0; i < 6; i++) {
                display->drawMenuItem(items[i], i, i == menuIndex);
            }
            display->drawScrollBar(6, 0, 6);
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
        } else if (currentState == STATE_SECURITY) {
            display->drawMenuItem("LockBoot: " + getBoolStr(data.securityLockOnBoot), 0, menuIndex == 0);
            display->drawMenuItem("Edit PIN", 1, menuIndex == 1);
            display->drawMenuItem("Back", 2, menuIndex == 2);
            display->drawScrollBar(3, 0, 5);
        } else if (currentState == STATE_SECURITY_PIN) {
            display->drawMenuItem("Digit 1: " + String(tempPin[0]), 0, menuIndex == 0);
            display->drawMenuItem("Digit 2: " + String(tempPin[1]), 1, menuIndex == 1);
            display->drawMenuItem("Digit 3: " + String(tempPin[2]), 2, menuIndex == 2);
            display->drawMenuItem("Digit 4: " + String(tempPin[3]), 3, menuIndex == 3);
            display->drawMenuItem("Save", 4, menuIndex == 4);
            display->drawMenuItem("Back", 5, menuIndex == 5);
            display->drawScrollBar(6, 0, 6);
        }
        display->updateMenu();
    }

    bool handleInput(uint8_t button) override {
        extern DisplayManager displayManager;
        ConfigData& data = ConfigManager::getInstance().data;

        if (button == 0) { // Scroll Up
            int maxItems = 0;
            if (currentState == STATE_MAIN) maxItems = 6;
            else if (currentState == STATE_DISPLAY) maxItems = 3;
            else if (currentState == STATE_WIFI) maxItems = 4;
            else if (currentState == STATE_BADUSB) maxItems = 4;
            else if (currentState == STATE_TIME) maxItems = 4;
            else if (currentState == STATE_SECURITY) maxItems = 3;
            else if (currentState == STATE_SECURITY_PIN) maxItems = 6;
            
            if (menuIndex > 0) menuIndex--;
            else menuIndex = maxItems - 1;
            drawMenu(&displayManager);
            return true;
        }

        if (button == 1) { // Scroll
            int maxItems = 0;
            if (currentState == STATE_MAIN) maxItems = 6;
            else if (currentState == STATE_DISPLAY) maxItems = 3;
            else if (currentState == STATE_WIFI) maxItems = 4;
            else if (currentState == STATE_BADUSB) maxItems = 4;
            else if (currentState == STATE_TIME) maxItems = 4;
            else if (currentState == STATE_SECURITY) maxItems = 3;
            else if (currentState == STATE_SECURITY_PIN) maxItems = 6;
            
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
                    currentState = STATE_SECURITY;
                    menuIndex = 0;
                }
                else if (menuIndex == 5) { 
                    ConfigManager::getInstance().save(); 
                    return false; // Exit module
                }
            }
            else if (currentState == STATE_DISPLAY) {
                if (menuIndex == 0) { // Brightness
                    data.displayBrightness = (data.displayBrightness + 32);
                    if (data.displayBrightness > 255) data.displayBrightness = 32;
                    displayManager.setBrightness(data.displayBrightness);
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
            else if (currentState == STATE_SECURITY) {
                if (menuIndex == 0) { // LockBoot
                    data.securityLockOnBoot = !data.securityLockOnBoot;
                }
                else if (menuIndex == 1) { // Edit PIN
                    currentState = STATE_SECURITY_PIN;
                    menuIndex = 0;
                    // Load current PIN into tempPin
                    String p = data.securityPin;
                    while(p.length() < 4) p += "0";
                    for(int i=0; i<4; i++) tempPin[i] = p[i];
                    tempPin[4] = '\0';
                }
                else if (menuIndex == 2) { // Back
                    currentState = STATE_MAIN;
                    menuIndex = 4;
                }
            }
            else if (currentState == STATE_SECURITY_PIN) {
                if (menuIndex >= 0 && menuIndex < 4) { // Edit Digit
                    char c = tempPin[menuIndex];
                    c++;
                    if (c > '9') c = '0';
                    tempPin[menuIndex] = c;
                }
                else if (menuIndex == 4) { // Save
                    data.securityPin = String(tempPin);
                    currentState = STATE_SECURITY;
                    menuIndex = 1;
                }
                else if (menuIndex == 5) { // Back
                    currentState = STATE_SECURITY;
                    menuIndex = 1;
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
