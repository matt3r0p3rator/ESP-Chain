#include "config_manager.h"

ConfigManager::ConfigManager(SDManager* sd) : sdManager(sd) {}

bool ConfigManager::loadConfig() {
    String json = sdManager->readFile("/config.json");
    if (json == "") {
        config.deviceName = "ESP-Chain";
        config.displayBrightness = 128;
        return false;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        config.deviceName = "ESP-Chain";
        config.displayBrightness = 128;
        return false;
    }

    config.deviceName = doc["device"]["name"] | "ESP-Chain";
    config.displayBrightness = doc["display"]["brightness"] | 128;
    
    return true;
}

Config ConfigManager::getConfig() {
    return config;
}
