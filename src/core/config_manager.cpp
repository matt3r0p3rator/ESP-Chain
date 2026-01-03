#include "config_manager.h"

bool ConfigManager::load() {
    Preferences prefs;
    
    // General Settings
    if (prefs.begin("settings", true)) { // Read-only
        // Display
        data.displayBrightness = prefs.getInt("disp_bright", 128);
        data.displayTimeout = prefs.getInt("disp_timeout", -1);
        data.displayTheme = prefs.getString("disp_theme", "purple_black");

        // WiFi
        data.wifiAutoScan = prefs.getBool("wifi_autoscan", true);
        data.wifiSaveHandshakes = prefs.getBool("wifi_savehs", true);
        data.wifiDeauthReason = prefs.getInt("wifi_reason", 7);
        data.wifiStorageSSID = prefs.getString("wifi_ssid", "ESP-Chain-Files");
        data.wifiStoragePassword = prefs.getString("wifi_pass", "password");

        // BadUSB
        data.badusbDelay = prefs.getInt("bad_delay", 100);
        data.badusbStartupDelay = prefs.getInt("bad_startdly", 2000);
        data.badusbAutoExec = prefs.getBool("bad_autoexec", false);
        
        prefs.end();
    }

    // Security Settings (match main.cpp)
    if (prefs.begin("security", true)) {
        data.securityPin = prefs.getString("device_pin", "0000");
        data.securityLockOnBoot = prefs.getBool("enable_pin_check", false);
        prefs.end();
    }

    return true;
}

bool ConfigManager::save() {
    Preferences prefs;
    
    // General Settings
    if (prefs.begin("settings", false)) { // Read-write
        // Display
        prefs.putInt("disp_bright", data.displayBrightness);
        prefs.putInt("disp_timeout", data.displayTimeout);
        prefs.putString("disp_theme", data.displayTheme);

        // WiFi
        prefs.putBool("wifi_autoscan", data.wifiAutoScan);
        prefs.putBool("wifi_savehs", data.wifiSaveHandshakes);
        prefs.putInt("wifi_reason", data.wifiDeauthReason);
        prefs.putString("wifi_ssid", data.wifiStorageSSID);
        prefs.putString("wifi_pass", data.wifiStoragePassword);

        // BadUSB
        prefs.putInt("bad_delay", data.badusbDelay);
        prefs.putInt("bad_startdly", data.badusbStartupDelay);
        prefs.putBool("bad_autoexec", data.badusbAutoExec);
        
        prefs.end();
    }

    // Security Settings
    if (prefs.begin("security", false)) {
        prefs.putString("device_pin", data.securityPin);
        prefs.putBool("enable_pin_check", data.securityLockOnBoot);
        prefs.end();
    }

    return true;
}
