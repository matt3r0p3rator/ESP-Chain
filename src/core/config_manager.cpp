#include "config_manager.h"

bool ConfigManager::load(String path) {
    extern SDManager sdManager;
    String json = sdManager.readFile(path);
    if (json.length() == 0) return false;

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, json);
    if (error) return false;

    // Display
    if (doc.containsKey("display")) {
        JsonObject display = doc["display"];
        data.displayBrightness = display["brightness"] | 128;
        data.displayTimeout = display["timeout_seconds"] | -1;
        const char* theme = display["theme"] | "purple_black";
        data.displayTheme = String(theme);
    }

    // WiFi
    if (doc.containsKey("wifi")) {
        JsonObject wifi = doc["wifi"];
        data.wifiAutoScan = wifi["auto_scan"] | true;
        data.wifiSaveHandshakes = wifi["save_handshakes"] | true;
        data.wifiDeauthReason = wifi["deauth_reason"] | 7;
    }

    // BadUSB
    if (doc.containsKey("badusb")) {
        JsonObject badusb = doc["badusb"];
        data.badusbDelay = badusb["default_delay_ms"] | 100;
        data.badusbStartupDelay = badusb["startup_delay_ms"] | 2000;
        data.badusbAutoExec = badusb["auto_execute"] | false;
    }

    return true;
}

bool ConfigManager::save(String path) {
    extern SDManager sdManager;
    
    DynamicJsonDocument doc(2048);
    
    // Try to preserve existing structure
    String currentJson = sdManager.readFile(path);
    if (currentJson.length() > 0) {
        deserializeJson(doc, currentJson);
    }

    // Update fields
    // Note: createNestedObject will return existing object if it exists, or create new.
    JsonObject display = doc["display"];
    if (display.isNull()) display = doc.createNestedObject("display");
    display["brightness"] = data.displayBrightness;
    display["timeout_seconds"] = data.displayTimeout;
    display["theme"] = data.displayTheme;

    JsonObject wifi = doc["wifi"];
    if (wifi.isNull()) wifi = doc.createNestedObject("wifi");
    wifi["auto_scan"] = data.wifiAutoScan;
    wifi["save_handshakes"] = data.wifiSaveHandshakes;
    wifi["deauth_reason"] = data.wifiDeauthReason;

    JsonObject badusb = doc["badusb"];
    if (badusb.isNull()) badusb = doc.createNestedObject("badusb");
    badusb["default_delay_ms"] = data.badusbDelay;
    badusb["startup_delay_ms"] = data.badusbStartupDelay;
    badusb["auto_execute"] = data.badusbAutoExec;

    String output;
    serializeJsonPretty(doc, output);
    return sdManager.writeFile(path, output);
}
