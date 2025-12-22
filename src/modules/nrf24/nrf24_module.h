#pragma once
#include <Arduino.h>
#include <vector>
#include "module_base.h"
#include "display_manager.h"
#include "../../ui/icons.h"


class NRF24Module : public Module {
private:
    enum State {
        MENU,
        SCANNER,
        SNIFFER,
        INJECTOR,
        SETTINGS,
        VIEW_RESULTS
    };
    State currentState;
    int menuIndex;
    int selectedIndex;
    bool isScanning;
    std::vector<String> scanResults;
    
public:
    void init() override {
        // Initialization code for NRF24 module
        currentState = MENU;
        menuIndex = 0;
        selectedIndex = 0;
        isScanning = false;
        scanResults.clear();

    }
    void loop() override {
        if (currentState == SCANNER && isScanning) {
            // Perform scanning operations
        }
    }
    String getName() override {
        return "NRF24 Tools";
    }
    const unsigned char* getIcon() override { return image_music_radio_streaming_bits; }
    int getIconWidth() override { return 17; }
    int getIconHeight() override { return 16; }
    int getIconSpacing() override { return 14 ; }
    int getIconOffsetY() override { return 1; }

    String getDescription() override {
        return "NRF24L01+ Utilities";
    }
    void drawMenu(DisplayManager* display) override {
        display->clearContent();
        extern SDManager sdManager;

        String title = "NRF24 Tools";
        switch(currentState) {
            case MENU: title = "NRF24 Tools"; break;
            case SCANNER: title = "NRF24 Scanner"; break;
            case VIEW_RESULTS: title = "Scan Results"; break;
            case SNIFFER: title = "NRF24 Sniffer"; break;
            case INJECTOR: title = "NRF24 Injector"; break;
            case SETTINGS: title = "NRF24 Settings"; break;
        }
        // We don't have easy access to wifi status here, so assume false or pass it if we change the interface.
        // For now, just pass false.
        display->drawStatusBar(title, display->getBatteryVoltage(), sdManager.isMounted(), false);
        
        switch(currentState) {
            case MENU:
                display->drawMenuTitle("NRF24 Tools");
                // Draw menu items
                display->drawMenuItem("Scanner", 0, menuIndex == 0);
                display->drawMenuItem("Sniffer", 1, menuIndex == 1);
                display->drawMenuItem("Injector", 2, menuIndex == 2);
                display->drawMenuItem("Settings", 3, menuIndex == 3);
                break;
            case SCANNER:
                display->drawMenuTitle("NRF24 Scanner");
                display->drawMenuItem(isScanning ? "Stop Scan" : "Start Scan", 0, menuIndex == 0);
                if (isScanning) {
                    display->getTFT()->setTextDatum(MC_DATUM);
                    display->getTFT()->setTextColor(TFT_GREEN, THEME_BG);
                    display->getTFT()->drawString("Scanning...", 160, 160, 2);
                }
                display->drawMenuItem("View Results", 1, menuIndex == 1);
                break;
            case VIEW_RESULTS:
                display->drawMenuTitle("Scan Results");
                if (scanResults.empty() && !isScanning) {
                    display->getTFT()->setTextDatum(MC_DATUM);
                    display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
                    display->getTFT()->drawString("No Devices Found", 160, 100, 2);
                    display->getTFT()->setTextDatum(MC_DATUM);
                    display->getTFT()->setTextColor(TFT_RED, THEME_BG);
                    display->getTFT()->drawString("Warning: Not Scanning", 160, 130, 2);
                    display->getTFT()->setTextDatum(MC_DATUM);
                    display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
                } else if (scanResults.empty()) {
                    display->getTFT()->setTextDatum(MC_DATUM);
                    display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
                    display->getTFT()->drawString("Scanning...", 160, 100, 2);
                } else {
                    // Show 5 items centered around selectedIndex
                    int start = 0;
                    if (selectedIndex > 2) start = selectedIndex - 2;
                    if (start + 5 > scanResults.size()) start = scanResults.size() - 5;
                    if (start < 0) start = 0;

                    for (int i = 0; i < 5 && (start + i) < scanResults.size(); i++) {
                        int idx = start + i;
                        String label = scanResults[idx];
                        display->drawMenuItem(label, i, idx == selectedIndex);
                    }
                }
                break;
            case SNIFFER:
                display->drawMenuTitle("NRF24 Sniffer");
                // Additional drawing for sniffer
                break;
            case INJECTOR:
                display->drawMenuTitle("NRF24 Injector");
                // Additional drawing for injector
                break;
            case SETTINGS:
                display->drawMenuTitle("NRF24 Settings");
                // Additional drawing for settings
                break;
        }
    }
    bool handleInput(uint8_t button) override {
        extern DisplayManager displayManager;

        if (button == 3) { // Back / Long Press
            if (currentState == MENU) {
                return false; // Exit module
            } else {
                switch (currentState) {
                    case SCANNER:
                        currentState = MENU;
                        isScanning = false; // Stop scanning if active
                        break;
                    case VIEW_RESULTS:
                        currentState = SCANNER;
                        break;
                    case SNIFFER:
                    case INJECTOR:
                    case SETTINGS:
                    default:
                        currentState = MENU;
                        break;
                }
            }
            drawMenu(&displayManager);
            return true;
        }

        if (button == 1) { // Scroll (Single Click)
            switch (currentState) {
                case MENU:
                    menuIndex = (menuIndex + 1) % 4; // 4 menu items
                    break;
                case SCANNER:
                    menuIndex = (menuIndex + 1) % 2; // 2 items: Scan toggle, View Results
                    break;
                case VIEW_RESULTS:
                    if (!scanResults.empty()) {
                        selectedIndex = (selectedIndex + 1) % scanResults.size();
                    }
                    break;
                default:
                    break;
            }
            drawMenu(&displayManager);
            return true;
        }

        if (button == 2) { // Select (Double Click)
            if (currentState == MENU) {
                switch(menuIndex) {
                    case 0:
                        currentState = SCANNER;
                        break;
                    case 1:
                        currentState = SNIFFER;
                        break;
                    case 2:
                        currentState = INJECTOR;
                        break;
                    case 3:
                        currentState = SETTINGS;
                        break;
                }
            } else if (currentState == SCANNER) {
                switch(menuIndex) {
                    case 0:
                        isScanning = !isScanning;
                        if (isScanning) {
                            // Start scanning
                        } else {
                            // Stop scanning
                        }
                        break;
                    case 1:
                        currentState = VIEW_RESULTS;
                        selectedIndex = 0;
                        break;
                    }
                }
            drawMenu(&displayManager);
            return true;
        }
        return true;
    }
};