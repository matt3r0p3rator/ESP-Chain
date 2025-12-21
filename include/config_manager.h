#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "sd_manager.h"

struct ConfigData {
    // Display
    int displayBrightness = 128;
    int displayTimeout = -1; // -1 = always on
    String displayTheme = "purple_black";
    
    // WiFi
    bool wifiAutoScan = true;
    bool wifiSaveHandshakes = true;
    int wifiDeauthReason = 7;

    // BadUSB
    int badusbDelay = 100;
    int badusbStartupDelay = 2000; // Delay before running payload after arming/plugin
    bool badusbAutoExec = false;
};

class ConfigManager {
public:
    ConfigData data;
    
    bool load(String path = "/config.json");
    bool save(String path = "/config.json");

    static ConfigManager& getInstance() {
        static ConfigManager instance;
        return instance;
    }
};
