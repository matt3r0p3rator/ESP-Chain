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
        VIEW_SCAN
    };
    State currentState;
    int menuIndex;
    int selectedIndex;
    int scrollOffset;
    bool isScanning;
    DisplayManager* displayManager;

    // Scanning variables
    struct NetworkInfo {
        String ssid;
        int32_t rssi;
        wifi_auth_mode_t encryption;
        int channel;
    };
    
    int networkCount = 0;
    std::vector<NetworkInfo> networks;
    
    // Helper methods
    String getEncryptionType(wifi_auth_mode_t encryption);
    String getSignalStrength(int32_t rssi);
    
public:
    WiFiModule() : displayManager(nullptr) {}
    void init() override;
    void loop() override;
    String getName() override { 
        if (currentState == VIEW_SCAN) return "Scan Results";
        return "WiFi Tools"; 
    }
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
