#pragma once
#include "module_base.h"
#include "display_manager.h"
#include "sd_manager.h"
#include <Update.h>
#include <vector>

extern SDManager sdManager;

class FirmwareUpgradeModule : public Module {
private:
    std::vector<FileEntry> firmwareFiles;
    int selectedIndex = 0;
    bool isUpdating = false;
    String statusMessage = "";
    
    void scanForFirmware() {
        firmwareFiles.clear();
        if (!sdManager.isMounted()) {
            if (!sdManager.init()) {
                statusMessage = "SD Init Failed";
                return;
            }
        }
        
        std::vector<FileEntry> allFiles = sdManager.listDir("/");
        for (const auto& file : allFiles) {
            if (!file.isDirectory && file.name.endsWith(".bin")) {
                firmwareFiles.push_back(file);
            }
        }
        
        if (firmwareFiles.empty()) {
            statusMessage = "No .bin files found";
        } else {
            statusMessage = "Select Firmware";
        }
        selectedIndex = 0;
    }

    void performUpdate(String fileName) {
        isUpdating = true;
        statusMessage = "Updating...";
        
        // Ensure leading slash
        if (!fileName.startsWith("/")) fileName = "/" + fileName;

        File file = SD.open(fileName);
        if (!file) {
            statusMessage = "File open failed";
            isUpdating = false;
            return;
        }

        size_t fileSize = file.size();
        if (!Update.begin(fileSize, U_FLASH)) {
            statusMessage = "Update begin failed";
            file.close();
            isUpdating = false;
            return;
        }

        size_t written = Update.writeStream(file);
        if (written == fileSize) {
            if (Update.end()) {
                if (Update.isFinished()) {
                    statusMessage = "Success! Rebooting...";
                } else {
                    statusMessage = "Update not finished";
                }
            } else {
                statusMessage = "Error: " + String(Update.getError());
            }
        } else {
            statusMessage = "Write failed";
        }
        
        file.close();
        isUpdating = false;
    }

public:
    void init() override {
        scanForFirmware();
    }

    void loop() override {
        if (statusMessage == "Success! Rebooting...") {
             delay(2000);
             ESP.restart();
        }
    }

    String getName() override {
        return "FW Upgrade";
    }
    
    const unsigned char* getIcon() override { return nullptr; } 

    String getDescription() override {
        return "Upgrade from SD";
    }

    void drawMenu(DisplayManager* display) override {
        display->clearContent();
        display->drawMenuTitle("Firmware Upgrade");
        
        display->getTFT()->setTextDatum(MC_DATUM);
        display->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
        
        if (isUpdating) {
            display->getTFT()->drawString("Updating...", 160, 120, 4);
            return;
        }
        
        if (statusMessage == "Success! Rebooting...") {
             display->getTFT()->setTextColor(TFT_GREEN, TFT_BLACK);
             display->getTFT()->drawString(statusMessage, 160, 120, 4);
             return;
        }

        if (firmwareFiles.empty()) {
            display->getTFT()->drawString(statusMessage, 160, 120, 4);
            return;
        }

        // Draw list
        int startY = 60;
        int lineHeight = 25;
        int maxItems = 5;
        int startIdx = 0;
        
        if (selectedIndex >= maxItems) {
            startIdx = selectedIndex - maxItems + 1;
        }
        
        if (startIdx < 0) startIdx = 0;

        for (int i = 0; i < min((int)firmwareFiles.size() - startIdx, maxItems); i++) {
            int idx = startIdx + i;
            if (idx >= firmwareFiles.size()) break;

            String name = firmwareFiles[idx].name;
            if (name.startsWith("/")) name = name.substring(1); 
            
            if (idx == selectedIndex) {
                display->getTFT()->setTextColor(TFT_GREEN, TFT_BLACK);
                name = "> " + name;
            } else {
                display->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
                name = "  " + name;
            }
            display->getTFT()->drawString(name, 160, startY + (i * lineHeight), 2);
        }
        
        display->getTFT()->setTextColor(TFT_YELLOW, TFT_BLACK);
        display->getTFT()->drawString("1:Next 2:Select 3:Back", 160, 220, 2);
        
        if (statusMessage != "Select Firmware" && statusMessage != "") {
             display->getTFT()->setTextColor(TFT_RED, TFT_BLACK);
             display->getTFT()->drawString(statusMessage, 160, 40, 2);
        }
    }

    bool handleInput(uint8_t button) override {
        if (isUpdating) return true;
        if (statusMessage == "Success! Rebooting...") return true;

        extern DisplayManager displayManager;

        if (button == 0) { // Up
            if (!firmwareFiles.empty()) {
                if (selectedIndex > 0) selectedIndex--;
                else selectedIndex = firmwareFiles.size() - 1;
                drawMenu(&displayManager);
            }
        } else if (button == 1) { // Next
            if (!firmwareFiles.empty()) {
                selectedIndex = (selectedIndex + 1) % firmwareFiles.size();
                drawMenu(&displayManager);
            }
        } else if (button == 2) { // Select
            if (!firmwareFiles.empty()) {
                isUpdating = true;
                statusMessage = "Updating...";
                drawMenu(&displayManager);
                delay(100);
                performUpdate(firmwareFiles[selectedIndex].name);
                drawMenu(&displayManager);
            }
        } else if (button == 3) { // Back
            return false;
        }
        return true;
    }
};
