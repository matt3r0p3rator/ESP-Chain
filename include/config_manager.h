#pragma once
#include <ArduinoJson.h>
#include "sd_manager.h"

struct Config {
    String deviceName;
    int displayBrightness;
};

class ConfigManager {
public:
    ConfigManager(SDManager* sd);
    bool loadConfig();
    Config getConfig();

private:
    SDManager* sdManager;
    Config config;
};
