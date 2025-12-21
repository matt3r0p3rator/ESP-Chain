#pragma once
#include <Arduino.h>
#include <vector>
#include <WiFi.h>
#include "module_base.h"
#include "display_manager.h"

struct APInfo {
    String ssid;
    int32_t rssi;
    uint8_t channel;
    String bssid;
    wifi_auth_mode_t encryption;
};

class WiFiModule : public Module {
private:
    enum State {
        MENU,
        SCANNING,
        RESULTS,
        TARGET_OPTIONS,
        ATTACK_DEAUTH,
        SETTINGS,
        SETTINGS_SCAN_TIME
    };

    State currentState;
    std::vector<APInfo> scanResults;
    int selectedIndex;
    int menuIndex;
    APInfo selectedTarget;
    bool isScanning;
    unsigned long lastUpdate;
    uint32_t scanTimePerChannel = 300; // Default 300ms

    const char* menuItems[2] = {"Scan Networks", "Settings"}; // Updated menu items

public:
    void init() override {
        currentState = MENU;
        menuIndex = 0;
        selectedIndex = 0;
        isScanning = false;
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
    }

    void loop() override {
        // Handle async scanning if needed, but for now we'll do blocking scan in init/transition
    }

    String getName() override {
        return "WiFi";
    }

    String getDescription() override {
        return "Scan & Attack";
    }

    void drawMenu(DisplayManager* display) override {
        display->clearContent();

        switch (currentState) {
            case MENU:
                display->drawMenuTitle("WiFi Menu");
                for (int i = 0; i < 2; i++) { 
                    display->drawMenuItem(menuItems[i], i, i == menuIndex);
                }
                break;

            case SETTINGS:
                display->drawMenuTitle("Settings");
                display->drawMenuItem("Scan Time", 0, true);
                break;

            case SETTINGS_SCAN_TIME:
                display->drawMenuTitle("Scan Time");
                display->getTFT()->setTextDatum(MC_DATUM);
                display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
                display->getTFT()->drawString(String(scanTimePerChannel) + " ms", 160, 100, 4);
                display->getTFT()->drawString("Single: +100ms", 160, 140, 2);
                display->getTFT()->drawString("Double: Save", 160, 160, 2);
                break;

            case SCANNING:
                display->drawMenuTitle("Scanning...");
                display->getTFT()->setTextDatum(MC_DATUM);
                display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
                display->getTFT()->drawString("Please Wait", 160, 100, 4);
                break;

            case RESULTS:
                display->clearContent();
                display->drawMenuTitle("Select Target");
                if (scanResults.empty()) {
                    display->getTFT()->setTextDatum(MC_DATUM);
                    display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
                    display->getTFT()->drawString("No Networks Found", 160, 100, 2);
                } else {
                    // Show 5 items centered around selectedIndex
                    int start = 0;
                    if (selectedIndex > 2) start = selectedIndex - 2;
                    if (start + 5 > scanResults.size()) start = scanResults.size() - 5;
                    if (start < 0) start = 0;

                    for (int i = 0; i < 5 && (start + i) < scanResults.size(); i++) {
                        int idx = start + i;
                        String label = scanResults[idx].ssid + " (" + String(scanResults[idx].rssi) + ")";
                        display->drawMenuItem(label, i, idx == selectedIndex);
                    }
                }
                break;

            case TARGET_OPTIONS:
                display->drawMenuTitle(selectedTarget.ssid);
                display->drawMenuItem("Deauth Attack", 0, true); // Only one option for now
                break;

            case ATTACK_DEAUTH:
                display->drawMenuTitle("Deauth Attack");
                display->getTFT()->setTextDatum(MC_DATUM);
                display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
                display->getTFT()->drawString("Target:", 160, 60, 2);
                display->getTFT()->drawString(selectedTarget.ssid, 160, 85, 4);
                display->getTFT()->drawString("Sending Packets...", 160, 120, 2);
                display->getTFT()->drawString("Long Press to Stop", 160, 150, 2);
                
                // Simulate attack loop here or in loop()
                // For visual feedback
                display->getTFT()->fillCircle(160, 180, 5, (millis() / 500) % 2 == 0 ? TFT_RED : TFT_BLACK);
                break;
        }
    }

    bool handleInput(uint8_t button) override {
        extern DisplayManager displayManager;

        if (button == 3) { // Back / Long Press
            switch (currentState) {
                case MENU:
                    return false; // Exit module
                case SETTINGS:
                    currentState = MENU;
                    break;
                case SETTINGS_SCAN_TIME:
                    currentState = SETTINGS; // Cancel changes? Or just go back
                    break;
                case SCANNING:
                    currentState = MENU;
                    break;
                case RESULTS:
                    currentState = MENU;
                    break;
                case TARGET_OPTIONS:
                    currentState = RESULTS;
                    break;
                case ATTACK_DEAUTH:
                    currentState = TARGET_OPTIONS;
                    break;
            }
            drawMenu(&displayManager);
            return true;
        }

        if (button == 1) { // Scroll (Single Click)
            switch (currentState) {
                case MENU:
                    menuIndex = (menuIndex + 1) % 2; 
                    break;
                case SETTINGS_SCAN_TIME:
                    scanTimePerChannel += 100;
                    if (scanTimePerChannel > 1000) scanTimePerChannel = 100; // Wrap around
                    break;
                case RESULTS:
                    if (!scanResults.empty()) {
                        selectedIndex = (selectedIndex + 1) % scanResults.size();
                    }
                    break;
                case TARGET_OPTIONS:
                    // Only 1 option
                    break;
                case ATTACK_DEAUTH:
                    // Do nothing
                    break;
                case SETTINGS:
                    // Only 1 option
                    break;
            }
            drawMenu(&displayManager);
            return true;
        }

        if (button == 2) { // Select (Double Click)
            switch (currentState) {
                case MENU:
                    if (menuIndex == 0) { // Scan
                        currentState = SCANNING;
                        drawMenu(&displayManager);
                        performScan();
                        currentState = RESULTS;
                        selectedIndex = 0;
                    } else if (menuIndex == 1) { // Settings
                        currentState = SETTINGS;
                    }
                    break;
                case SETTINGS:
                    currentState = SETTINGS_SCAN_TIME;
                    break;
                case SETTINGS_SCAN_TIME:
                    currentState = SETTINGS; // Save and go back
                    break;
                case RESULTS:
                    if (!scanResults.empty()) {
                        selectedTarget = scanResults[selectedIndex];
                        currentState = TARGET_OPTIONS;
                    }
                    break;
                case TARGET_OPTIONS:
                    currentState = ATTACK_DEAUTH;
                    break;
                case ATTACK_DEAUTH:
                    // Maybe toggle?
                    break;
            }
            drawMenu(&displayManager);
            return true;
        }
        return true;
    }

private:
    void performScan() {
        scanResults.clear();
        // async=false, show_hidden=true, passive=false, max_ms_per_channel=scanTimePerChannel
        int n = WiFi.scanNetworks(false, true, false, scanTimePerChannel);
        for (int i = 0; i < n; ++i) {
            APInfo ap;
            ap.ssid = WiFi.SSID(i);
            ap.rssi = WiFi.RSSI(i);
            ap.channel = WiFi.channel(i);
            ap.bssid = WiFi.BSSIDstr(i);
            ap.encryption = WiFi.encryptionType(i);
            scanResults.push_back(ap);
        }
    }
};
