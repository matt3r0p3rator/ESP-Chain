#include "config_manager.h"
#include "Preferences.h"

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
        const char* ssid = wifi["storage_ssid"] | "ESP-Chain-Files";
        data.wifiStorageSSID = String(ssid);
        const char* pass = wifi["storage_password"] | "password";
        data.wifiStoragePassword = String(pass);
    }

    // BadUSB
    if (doc.containsKey("badusb")) {
        JsonObject badusb = doc["badusb"];
        data.badusbDelay = badusb["default_delay_ms"] | 100;
        data.badusbStartupDelay = badusb["startup_delay_ms"] | 2000;
        data.badusbAutoExec = badusb["auto_execute"] | false;
    }    

    // Security
    Preferences prefs;
    prefs.begin("security", true);
    data.securityPin = prefs.getString("pin", "0000");
    data.securityLockOnBoot = prefs.getBool("lock_on_boot", false);
    prefs.end();

    // Game High Scores
    if (doc.containsKey("games")) {
        JsonObject games = doc["games"];
        data.flappyBirdHighScore = games["flappy_bird_high_score"] | 0;
        data.snakeHighScore = games["snake_high_score"] | 0;
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
    wifi["storage_ssid"] = data.wifiStorageSSID;
    wifi["storage_password"] = data.wifiStoragePassword;
    if (wifi.isNull()) wifi = doc.createNestedObject("wifi");
    wifi["auto_scan"] = data.wifiAutoScan;
    wifi["save_handshakes"] = data.wifiSaveHandshakes;
    wifi["deauth_reason"] = data.wifiDeauthReason;

    JsonObject badusb = doc["badusb"];
    if (badusb.isNull()) badusb = doc.createNestedObject("badusb");
    badusb["default_delay_ms"] = data.badusbDelay;
    badusb["startup_delay_ms"] = data.badusbStartupDelay;
    badusb["auto_execute"] = data.badusbAutoExec;

    Preferences prefs;
    prefs.begin("security", false);
    prefs.putString("pin", data.securityPin);
    prefs.putBool("lock_on_boot", data.securityLockOnBoot);
    prefs.end();

    // Save game high scores
    JsonObject games = doc["games"];
    if (games.isNull()) games = doc.createNestedObject("games");
    games["flappy_bird_high_score"] = data.flappyBirdHighScore;
    games["snake_high_score"] = data.snakeHighScore;

    String output;
    serializeJsonPretty(doc, output);
    return sdManager.writeFile(path, output);
}
