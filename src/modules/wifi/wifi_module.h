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
        VIEW_SCAN,
        NETWORK_ACTIONS,
        VIEW_DETAILS
    };
    State currentState;
    int menuIndex;
    int selectedIndex;
    String selectedSSID; // Track selected network by SSID
    String selectedBSSID; // Track selected network by BSSID (MAC address)
    int scrollOffset;
    int actionMenuIndex;
    bool isScanning;
    DisplayManager* displayManager;

    // Scanning variables
    struct NetworkInfo {
        String ssid;
        String bssid; // MAC address for unique identification
        int32_t rssi;
        wifi_auth_mode_t encryption;
        int channel;
    };
    
    int networkCount = 0;
    std::vector<NetworkInfo> networks;
    
    // Helper methods
    String getEncryptionType(wifi_auth_mode_t encryption);
    String getSignalStrength(int32_t rssi);
    void showNetworkDetails();
    void saveNetworkToSD();
    int getSelectedNetworkIndex(); // Get current index of selected network by SSID
    
public:
    WiFiModule() : displayManager(nullptr) {}
    void init() override;
    void loop() override;
    String getName() override { 
        if (currentState == VIEW_SCAN) return "Scan Results";
        else if (currentState == NETWORK_ACTIONS) return "Network Actions";
        else if (currentState == VIEW_DETAILS) return "Network Details";
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
