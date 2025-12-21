#pragma once
#include "module_base.h"
#include "sd_manager.h"
#include "display_manager.h"
#include <Update.h>
#include <vector>

class FirmwareUpgradeModule : public Module {
private:
    std::vector<FileEntry> binFiles;
    int selectedIndex = 0;
    bool isUpdating = false;
    String statusMessage = "";
    float progress = 0.0;

    void scanFiles() {
        extern SDManager sdManager;
        std::vector<FileEntry> allFiles = sdManager.listDir("/");
        binFiles.clear();
        for (const auto& file : allFiles) {
            if (!file.isDirectory && file.name.endsWith(".bin")) {
                binFiles.push_back(file);
            }
        }
        if (selectedIndex >= binFiles.size()) selectedIndex = 0;
    }

    void drawProgress(DisplayManager* display) {
        display->clearContent();
        display->drawMenuTitle("Updating...");
        display->getTFT()->setTextDatum(MC_DATUM);
        display->getTFT()->drawString(statusMessage, display->getTFT()->width() / 2, display->getTFT()->height() / 2 - 20);
        
        int w = 140;
        int h = 20;
        int x = (display->getTFT()->width() - w) / 2;
        int y = (display->getTFT()->height() / 2) + 10;
        
        display->getTFT()->drawRect(x, y, w, h, TFT_WHITE);
        display->getTFT()->fillRect(x + 2, y + 2, (w - 4) * progress, h - 4, TFT_GREEN);
        
        String pct = String((int)(progress * 100)) + "%";
        display->getTFT()->drawString(pct, display->getTFT()->width() / 2, y + h + 15);
    }

    void performUpdate(String filename) {
        extern DisplayManager displayManager;
        isUpdating = true;
        statusMessage = "Starting...";
        progress = 0.0;
        drawProgress(&displayManager);
        
        Serial.println("Starting Firmware Update");
        Serial.print("File: "); Serial.println(filename);

        // Ensure path starts with /
        String path = filename;
        if (!path.startsWith("/")) path = "/" + path;

        File file = SD.open(path);
        if (!file) {
            Serial.println("Failed to open file");
            statusMessage = "Failed to open file";
            drawProgress(&displayManager);
            delay(2000);
            isUpdating = false;
            return;
        }

        size_t fileSize = file.size();
        Serial.print("File Size: "); Serial.println(fileSize);
        
        if (fileSize == 0) {
            Serial.println("File is empty");
            statusMessage = "File is empty";
            drawProgress(&displayManager);
            file.close();
            delay(2000);
            isUpdating = false;
            return;
        }

        if (!Update.begin(fileSize, U_FLASH)) {
            Serial.print("Update.begin failed: "); Serial.println(Update.getError());
            statusMessage = "Begin failed: " + String(Update.getError());
            drawProgress(&displayManager);
            file.close();
            delay(2000);
            isUpdating = false;
            return;
        }

        size_t written = 0;
        uint8_t buffer[1024]; // 1KB buffer
        unsigned long lastDraw = 0;

        while (file.available()) {
            int len = file.read(buffer, sizeof(buffer));
            if (len > 0) {
                size_t w = Update.write(buffer, len);
                if (w != len) {
                    Serial.print("Write failed: "); Serial.println(Update.getError());
                    statusMessage = "Write failed: " + String(Update.getError());
                    drawProgress(&displayManager);
                    file.close();
                    delay(2000);
                    isUpdating = false;
                    return;
                }
                written += len;
                progress = (float)written / fileSize;
                
                // Update UI periodically
                if (millis() - lastDraw > 200) { // Slower updates
                    drawProgress(&displayManager);
                    lastDraw = millis();
                    Serial.print("."); // Keep serial alive
                }
                yield(); // Feed the watchdog
            } else {
                break;
            }
        }
        
        Serial.println();
        
        if (Update.end()) {
            if (Update.isFinished()) {
                Serial.println("Update Success");
                statusMessage = "Success! Rebooting...";
                progress = 1.0;
                drawProgress(&displayManager);
                delay(2000);
                ESP.restart();
            } else {
                Serial.println("Update not finished");
                statusMessage = "Update not finished";
                drawProgress(&displayManager);
                delay(2000);
            }
        } else {
            Serial.print("Update.end failed: "); Serial.println(Update.getError());
            statusMessage = "End failed: " + String(Update.getError());
            drawProgress(&displayManager);
            delay(2000);
        }
        
        file.close();
        isUpdating = false;
    }

public:
    void init() override {
        scanFiles();
    }

    void loop() override {
    }

    String getName() override {
        return "FW Upgrade";
    }

    String getDescription() override {
        return "Flash .bin from SD";
    }

    void drawMenu(DisplayManager* display) override {
        if (isUpdating) return; 

        display->clearContent();
        display->drawMenuTitle("Firmware Upgrade");
        
        if (binFiles.empty()) {
            display->getTFT()->setTextDatum(MC_DATUM);
            display->getTFT()->drawString("No .bin files found", display->getTFT()->width() / 2, display->getTFT()->height() / 2);
            return;
        }

        int y = 40;
        int itemHeight = 25;
        int maxItems = (display->getTFT()->height() - 40) / itemHeight;
        int startIdx = 0;
        if (selectedIndex >= maxItems) {
            startIdx = selectedIndex - maxItems + 1;
        }

        for (int i = 0; i < maxItems && (startIdx + i) < binFiles.size(); i++) {
            int idx = startIdx + i;
            bool selected = (idx == selectedIndex);
            display->drawMenuItem(binFiles[idx].name, i, selected);
        }
    }

    bool handleInput(uint8_t button) override {
        if (isUpdating) return true;

        if (binFiles.empty()) {
            if (button == 3) return false; // Back
            return true;
        }

        if (button == 1) { // Next
            selectedIndex++;
            if (selectedIndex >= binFiles.size()) selectedIndex = 0;
        } else if (button == 2) { // Select
            performUpdate(binFiles[selectedIndex].name);
        } else if (button == 3) { // Back
            return false;
        }
        
        return true;
    }
};
