#pragma once
#include <Arduino.h>
#include "module_base.h"
#include "display_manager.h"
#include "../../ui/icons.h"
#include <WiFi.h>
#include <vector>
class WiFiModule : public Module {
private:
    enum State {
        MENU,
        SCAN_NETWORKS,
        EVIL_PORTAL,
        KARMA_ATTACK,
        RESPONDER,
        SNIFFER,
        DEAUTH_ATTACK,
        CAPTURE_HANDSHAKE
    };
    State currentState;
    int menuIndex;
    int selectedIndex;
    int scrollOffset;
    DisplayManager* displayManager;

    // Scanning variables
    int networkCount = 0;
    bool scanComplete = false;
    bool isScanning = false;
    std::vector<String> scanResults;
    
public:
    WiFiModule() : displayManager(nullptr) {}
    void init() override;
    void loop() override;
    String getName() override { return "WiFi Tools"; }
    const unsigned char* getIcon() override { return image_wifi_bits; }
    int getIconWidth() override { return 19; }
    int getIconHeight() override { return 16; }
    int getIconSpacing() override { return 16 ; }
    int getIconOffsetY() override { return 0; }
    String getDescription() override { return "WiFi Penetration Testing Utilities"; }
    void drawMenu(DisplayManager* display) override;
    bool handleInput(uint8_t button) override;
    // Helper methods for various functionalities can be declared here
};
