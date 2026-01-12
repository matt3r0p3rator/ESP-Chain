#include "firmware_launcher.h"
#include <esp_system.h>
#include <FS.h>
#include <SD.h>

bool FirmwareLauncher::hasSecondaryFirmware() {
    const esp_partition_t* ota_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, 
        ESP_PARTITION_SUBTYPE_APP_OTA_1, 
        NULL
    );
    
    if (!ota_partition) {
        Serial.println("OTA_1 partition not found");
        return false;
    }
    
    // Read first byte to check if valid firmware exists
    uint8_t firstByte;
    esp_err_t err = esp_partition_read(ota_partition, 0, &firstByte, 1);
    
    if (err != ESP_OK) {
        Serial.println("Failed to read OTA partition");
        return false;
    }
    
    // ESP32 firmware starts with 0xE9
    return (firstByte == 0xE9);
}

bool FirmwareLauncher::bootSecondaryFirmware() {
    // Get current running partition
    const esp_partition_t* current = esp_ota_get_running_partition();
    if (current) {
        Serial.printf("Current partition: %s (subtype: %d)\n", current->label, current->subtype);
    }
    
    const esp_partition_t* ota_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, 
        ESP_PARTITION_SUBTYPE_APP_OTA_1, 
        NULL
    );
    
    if (!ota_partition) {
        Serial.println("OTA_1 partition not found");
        return false;
    }
    
    Serial.printf("OTA_1 partition found: %s at 0x%x, size: %d\n", 
                  ota_partition->label, ota_partition->address, ota_partition->size);
    
    if (!hasSecondaryFirmware()) {
        Serial.println("No valid secondary firmware");
        return false;
    }
    
    // Set the boot partition to OTA_1
    Serial.println("Setting boot partition to OTA_1...");
    esp_err_t err = esp_ota_set_boot_partition(ota_partition);
    if (err != ESP_OK) {
        Serial.printf("Failed to set boot partition: %d (%s)\n", err, esp_err_to_name(err));
        return false;
    }
    
    // Verify it was set
    const esp_partition_t* boot_partition = esp_ota_get_boot_partition();
    if (boot_partition) {
        Serial.printf("Boot partition now set to: %s\n", boot_partition->label);
    }
    
    Serial.println("Rebooting to secondary firmware in 1 second...");
    Serial.flush(); // Ensure all serial data is sent
    delay(1000);
    
    Serial.println("Restarting NOW...");
    Serial.flush();
    ESP.restart();
    
    // Should never reach here, but just in case
    while(1) {
        delay(100);
    }
    
    return true;
}

bool FirmwareLauncher::installFirmwareFromURL(const String& url, const String& ssid, const String& password) {
    // Connect to WiFi
    WiFi.begin(ssid.c_str(), password.c_str());
    
    Serial.print("Connecting to WiFi");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nWiFi connection failed");
        return false;
    }
    
    Serial.println("\nWiFi connected");
    
    const esp_partition_t* ota_partition = getOtaPartition();
    if (!ota_partition) {
        WiFi.disconnect();
        return false;
    }
    
    WiFiClientSecure client;
    client.setInsecure(); // Skip certificate validation
    
    HTTPClient http;
    http.begin(client, url);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("HTTP GET failed, error: %d\n", httpCode);
        http.end();
        WiFi.disconnect();
        return false;
    }
    
    int contentLength = http.getSize();
    Serial.printf("Firmware size: %d bytes\n", contentLength);
    
    if (contentLength <= 0 || contentLength > ota_partition->size) {
        Serial.println("Invalid content length");
        http.end();
        WiFi.disconnect();
        return false;
    }
    
    // Erase the partition first
    Serial.println("Erasing OTA partition...");
    esp_err_t err = esp_partition_erase_range(ota_partition, 0, ota_partition->size);
    if (err != ESP_OK) {
        Serial.printf("Partition erase failed: %d\n", err);
        http.end();
        WiFi.disconnect();
        return false;
    }
    
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[1024];
    size_t written = 0;
    size_t offset = 0;
    
    Serial.println("Writing firmware...");
    
    while (http.connected() && (written < contentLength)) {
        size_t available = stream->available();
        
        if (available) {
            size_t bytesToRead = min(available, sizeof(buffer));
            size_t bytesRead = stream->readBytes(buffer, bytesToRead);
            
            if (bytesRead > 0) {
                // Validate header on first chunk
                if (offset == 0 && !validateFirmwareHeader(buffer, bytesRead)) {
                    Serial.println("Invalid firmware header");
                    http.end();
                    WiFi.disconnect();
                    return false;
                }
                
                err = esp_partition_write(ota_partition, offset, buffer, bytesRead);
                if (err != ESP_OK) {
                    Serial.printf("Partition write failed: %d\n", err);
                    http.end();
                    WiFi.disconnect();
                    return false;
                }
                
                offset += bytesRead;
                written += bytesRead;
                
                if (progressCallback) {
                    progressCallback(written, contentLength);
                }
                
                if (written % 10240 == 0) {
                    Serial.printf("Written: %d / %d bytes\n", written, contentLength);
                }
            }
        }
        delay(1);
    }
    
    http.end();
    WiFi.disconnect();
    
    Serial.printf("Firmware installation complete: %d bytes written\n", written);
    return (written == contentLength);
}

bool FirmwareLauncher::installFirmwareFromSD(const String& filepath) {
    if (!SD.exists(filepath)) {
        Serial.println("Firmware file not found");
        return false;
    }
    
    File file = SD.open(filepath, FILE_READ);
    if (!file) {
        Serial.println("Failed to open firmware file");
        return false;
    }
    
    size_t fileSize = file.size();
    Serial.printf("Firmware file size: %d bytes\n", fileSize);
    
    const esp_partition_t* ota_partition = getOtaPartition();
    if (!ota_partition) {
        file.close();
        return false;
    }
    
    if (fileSize > ota_partition->size) {
        Serial.println("Firmware too large for partition");
        file.close();
        return false;
    }
    
    // Erase the partition first
    Serial.println("Erasing OTA partition...");
    esp_err_t err = esp_partition_erase_range(ota_partition, 0, ota_partition->size);
    if (err != ESP_OK) {
        Serial.printf("Partition erase failed: %d\n", err);
        file.close();
        return false;
    }
    
    uint8_t buffer[1024];
    size_t offset = 0;
    size_t bytesRead;
    
    Serial.println("Writing firmware from SD...");
    
    while ((bytesRead = file.read(buffer, sizeof(buffer))) > 0) {
        // Validate header on first chunk
        if (offset == 0 && !validateFirmwareHeader(buffer, bytesRead)) {
            Serial.println("Invalid firmware header");
            file.close();
            return false;
        }
        
        err = esp_partition_write(ota_partition, offset, buffer, bytesRead);
        if (err != ESP_OK) {
            Serial.printf("Partition write failed: %d\n", err);
            file.close();
            return false;
        }
        
        offset += bytesRead;
        
        if (progressCallback) {
            progressCallback(offset, fileSize);
        }
        
        if (offset % 10240 == 0) {
            Serial.printf("Written: %d / %d bytes\n", offset, fileSize);
        }
    }
    
    file.close();
    
    Serial.printf("Firmware installation complete: %d bytes written\n", offset);
    return (offset == fileSize);
}

String FirmwareLauncher::getSecondaryFirmwareInfo() {
    // Show current running partition
    const esp_partition_t* current = esp_ota_get_running_partition();
    String currentInfo = "Current: ";
    if (current) {
        currentInfo += String(current->label);
        currentInfo += " (";
        if (current->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_0) {
            currentInfo += "OTA_0";
        } else if (current->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1) {
            currentInfo += "OTA_1";
        } else if (current->subtype == ESP_PARTITION_SUBTYPE_APP_FACTORY) {
            currentInfo += "FACTORY";
        } else {
            currentInfo += String(current->subtype);
        }
        currentInfo += ")\\n";
    } else {
        currentInfo += "Unknown\\n";
    }
    
    if (!hasSecondaryFirmware()) {
        return currentInfo + "\\nNo secondary firmware installed\\nInstall .bin file from SD";
    }
    
    const esp_partition_t* ota_partition = getOtaPartition();
    if (!ota_partition) {
        return currentInfo + "\\nError reading partition";
    }
    
    char info[256];
    snprintf(info, sizeof(info), "%sSecondary: OTA_1\\nSize: %d KB\\nStatus: Ready to boot", 
             currentInfo.c_str(), ota_partition->size / 1024);
    
    return String(info);
}

bool FirmwareLauncher::eraseSecondaryFirmware() {
    const esp_partition_t* ota_partition = getOtaPartition();
    if (!ota_partition) {
        return false;
    }
    
    Serial.println("Erasing secondary firmware...");
    esp_err_t err = esp_partition_erase_range(ota_partition, 0, ota_partition->size);
    
    if (err != ESP_OK) {
        Serial.printf("Erase failed: %d\n", err);
        return false;
    }
    
    Serial.println("Secondary firmware erased");
    return true;
}

void FirmwareLauncher::setProgressCallback(ProgressCallback callback) {
    progressCallback = callback;
}

bool FirmwareLauncher::validateFirmwareHeader(uint8_t* data, size_t len) {
    if (len < 1) {
        return false;
    }
    
    // ESP32 firmware starts with 0xE9 magic byte
    return (data[0] == 0xE9);
}

const esp_partition_t* FirmwareLauncher::getOtaPartition() {
    const esp_partition_t* ota_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, 
        ESP_PARTITION_SUBTYPE_APP_OTA_1, 
        NULL
    );
    
    if (!ota_partition) {
        Serial.println("OTA_1 partition not found");
        return nullptr;
    }
    
    return ota_partition;
}
