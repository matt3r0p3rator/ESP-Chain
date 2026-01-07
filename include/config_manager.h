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
    String wifiStorageSSID = "ESP-Chain-Files";
    String wifiStoragePassword = "password";

    // BadUSB
    int badusbDelay = 100;
    int badusbStartupDelay = 2000; // Delay before running payload after arming/plugin
    bool badusbAutoExec = false;

    // Security
    String securityPin = "0000";
    bool securityLockOnBoot = false;

    // Game High Scores
    int flappyBirdHighScore = 0;
    int snakeHighScore = 0;

    int sleepTimeout = 30;
    bool sleepEnabled = true;
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
