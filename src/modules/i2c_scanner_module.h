#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <vector>
#include "module_base.h"
#include "display_manager.h"

// Default I2C pins for LilyGo T-Display S3
#ifndef SDA_PIN
#define SDA_PIN 43
#endif
#ifndef SCL_PIN
#define SCL_PIN 44
#endif

#define EXT_POWER_PIN 17

struct I2CDevice {
    uint8_t address;
    String hexAddress;
    String status;
};

class I2CScannerModule : public Module {
private:
    enum State {
        SCANNING,
        RESULTS
    };

    State currentState;
    std::vector<I2CDevice> devices;
    int selectedIndex;
    
    // Scanning variables
    int currentScanAddress;
    bool isScanning;

    void startScan() {
        currentState = SCANNING;
        isScanning = true;
        currentScanAddress = 1;
        devices.clear();
        selectedIndex = 0;

        // Power Cycle
        pinMode(EXT_POWER_PIN, OUTPUT);
        digitalWrite(EXT_POWER_PIN, LOW);
        delay(50);
        digitalWrite(EXT_POWER_PIN, HIGH);
        delay(100);

        Wire.begin(SDA_PIN, SCL_PIN);
        Wire.setClock(100000);
    }

public:
    void init() override {
        startScan();
    }

    void loop() override {
        if (isScanning) {
            extern DisplayManager displayManager;

            // Scan one address per loop
            Wire.beginTransmission(currentScanAddress);
            byte error = Wire.endTransmission();

            if (error == 0) {
                I2CDevice dev;
                dev.address = currentScanAddress;
                dev.hexAddress = "0x" + String(currentScanAddress, HEX);
                if (currentScanAddress < 16) dev.hexAddress = "0x0" + String(currentScanAddress, HEX);
                dev.status = "OK";
                devices.push_back(dev);
            }
            else if (error == 4) {
                I2CDevice dev;
                dev.address = currentScanAddress;
                dev.hexAddress = "0x" + String(currentScanAddress, HEX);
                dev.status = "Error";
                devices.push_back(dev);
            }

            currentScanAddress++;

            // Update display periodically
            if (currentScanAddress % 10 == 0 || currentScanAddress >= 127) {
                drawMenu(&displayManager);
            }

            if (currentScanAddress >= 127) {
                isScanning = false;
                currentState = RESULTS;
                drawMenu(&displayManager);
            }
        }
    }

    String getName() override {
        return "I2C Scanner";
    }

    String getDescription() override {
        return "List I2C Devices";
    }

    void drawMenu(DisplayManager* display) override {
        if (!display || !display->getTFT()) return;

        display->clearContent();

        if (currentState == SCANNING) {
            display->drawMenuTitle("Scanning...");
            display->getTFT()->setTextDatum(MC_DATUM);
            display->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
            
            int percent = (int)((currentScanAddress / 127.0) * 100);
            display->getTFT()->drawString(String(percent) + "%", 160, 90, 4);
            display->getTFT()->drawString("Found: " + String(devices.size()), 160, 130, 2);
        } 
        else if (currentState == RESULTS) {
            display->drawMenuTitle("I2C Devices");
            
            if (devices.empty()) {
                display->getTFT()->setTextDatum(MC_DATUM);
                display->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
                display->getTFT()->drawString("No Devices Found", 160, 100, 2);
                display->getTFT()->drawString("Btn 1: Rescan", 160, 140, 2);
            } else {
                // List View Logic
                int itemsPerPage = 5;
                int start = 0;
                if (selectedIndex > 2) start = selectedIndex - 2;
                if (start + itemsPerPage > devices.size()) start = devices.size() - itemsPerPage;
                if (start < 0) start = 0;

                for (int i = 0; i < itemsPerPage && (start + i) < devices.size(); i++) {
                    int idx = start + i;
                    String label = devices[idx].hexAddress + " - " + devices[idx].status;
                    display->drawMenuItem(label, i, idx == selectedIndex);
                }
            }
        }
    }

    bool handleInput(uint8_t button) override {
        extern DisplayManager displayManager;

        if (button == 3) { // Long Press -> Exit
            return false; 
        }

        if (currentState == RESULTS) {
            if (devices.empty()) {
                if (button == 1) { // Rescan
                    startScan();
                }
            } else {
                if (button == 1) { // Down / Next
                    selectedIndex++;
                    if (selectedIndex >= devices.size()) selectedIndex = 0;
                    drawMenu(&displayManager);
                }
                else if (button == 2) { // Select (Double Click) -> Maybe show details or rescan?
                    // For now, let's make double click rescan
                    startScan();
                }
            }
        }

        return true;
    }
};
