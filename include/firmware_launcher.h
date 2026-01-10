#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

class FirmwareLauncher {
public:
    static FirmwareLauncher& getInstance() {
        static FirmwareLauncher instance;
        return instance;
    }

    // Check if secondary firmware is installed
    bool hasSecondaryFirmware();
    
    // Boot into secondary firmware
    bool bootSecondaryFirmware();
    
    // Install firmware from URL
    bool installFirmwareFromURL(const String& url, const String& ssid, const String& password);
    
    // Install firmware from SD card file
    bool installFirmwareFromSD(const String& filepath);
    
    // Get secondary firmware info
    String getSecondaryFirmwareInfo();
    
    // Erase secondary firmware
    bool eraseSecondaryFirmware();
    
    // Progress callback
    typedef void (*ProgressCallback)(size_t current, size_t total);
    void setProgressCallback(ProgressCallback callback);

private:
    FirmwareLauncher() : progressCallback(nullptr) {}
    FirmwareLauncher(const FirmwareLauncher&) = delete;
    FirmwareLauncher& operator=(const FirmwareLauncher&) = delete;
    
    ProgressCallback progressCallback;
    
    bool validateFirmwareHeader(uint8_t* data, size_t len);
    const esp_partition_t* getOtaPartition();
};
