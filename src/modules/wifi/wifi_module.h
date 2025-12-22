#pragma once
#include <Arduino.h>
#include <vector>
#include <WiFi.h>
#include "module_base.h"
#include "display_manager.h"
#include "../../ui/icons.h"

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
        SCANNER_MENU,
        RESULTS,
        DETAILS,
        TARGET_OPTIONS,
        ATTACK_DEAUTH,
        SETTINGS,
        SETTINGS_SCAN_TIME,
        SETTINGS_SHOW_HIDDEN,
        SETTINGS_SORT_METHOD
    };

    enum SortMethod {
        SORT_RSSI,
        SORT_CHANNEL
    };

    State currentState;
    std::vector<APInfo> scanResults;
    int selectedIndex;
    int menuIndex;
    int settingsIndex;
    APInfo selectedTarget;
    bool isScanning;
    unsigned long lastUpdate;
    
    // Settings
    uint32_t scanTimePerChannel = 300;
    bool showHidden = true;
    SortMethod sortMethod = SORT_RSSI;

    const char* menuItems[2] = {"Scan Networks", "Settings"};
    const char* settingsItems[3] = {"Scan Time", "Show Hidden", "Sort By"};

public:
    void init() override;
    void loop() override;
    String getName() override;
    const unsigned char* getIcon() override;
    int getIconWidth() override;
    int getIconHeight() override;
    String getDescription() override;
    void drawMenu(DisplayManager* display) override;
    bool handleInput(uint8_t button) override;

private:
    void performScan();
    void sortResults();
    String getEncryptionName(wifi_auth_mode_t encryption);
};
