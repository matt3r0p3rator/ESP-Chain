#include "wifi_module.h"
#include "display_manager.h"

// Menu Items
const char* menuItems[] = {
    "Scan Results",
    "Evil Portal",
    "Karma Attack",
    "Responder",
    "Sniffer",
    "Deauth Attack",
    "Capture Handshake"
};
const int menuItemsCount = 7;


void WiFiModule::init() {
    Serial.println("[WiFi] Init started");
    currentState = MENU;
    menuIndex = 0;
    selectedIndex = 0;
    scrollOffset = 0;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    isScanning = false;
    scanComplete = false;
    scanDataReady = false;
    Serial.println("[WiFi] Init complete");
}

void WiFiModule::loop() {
    // Check if scanning is in progress
    if (isScanning) {
        int n = WiFi.scanComplete();
        if (n == WIFI_SCAN_RUNNING) {
            // Scan still running - check if we have partial results
            // ESP32 doesn't give partial results, so just return
            return;
        } else if (n >= 0) {
            Serial.print("[WiFi] Scan complete, networks found: ");
            Serial.println(n);
            
            // Scan complete
            isScanning = false;
            scanComplete = true;
            networkCount = n;
            
            Serial.println("[WiFi] Clearing networks vector");
            processingData = true; // Lock data during processing
            networks.clear();
            
            // Mark data as ready immediately - we'll update as we process
            scanDataReady = true;
            
            // Reserve space to avoid reallocation
            if (n > 0) {
                Serial.println("[WiFi] Reserving vector space");
                networks.reserve(n);
            }
            
            // Limit number of networks to prevent memory issues
            int maxNetworks = (n > 20) ? 20 : n;
            Serial.print("[WiFi] Processing ");
            Serial.print(maxNetworks);
            Serial.println(" networks");
            
            for (int i = 0; i < maxNetworks; i++) {
                Serial.print("[WiFi] Processing network #");
                Serial.println(i);
                
                NetworkInfo info;
                info.rssi = WiFi.RSSI(i);
                Serial.print("  RSSI: ");
                Serial.println(info.rssi);
                
                info.channel = WiFi.channel(i);
                Serial.print("  Channel: ");
                Serial.println(info.channel);
                
                info.encryption = WiFi.encryptionType(i);
                Serial.print("  Encryption: ");
                Serial.println(info.encryption);
                
                // Get SSID - be careful with String operations
                Serial.println("  Getting SSID...");
                String tempSSID = WiFi.SSID(i);
                Serial.print("  SSID length: ");
                Serial.println(tempSSID.length());
                
                if (tempSSID.length() == 0 || tempSSID.length() > 32) {
                    info.ssid = "<Hidden>";
                    Serial.println("  Using <Hidden>");
                } else {
                    info.ssid = tempSSID;
                    Serial.print("  SSID: ");
                    Serial.println(info.ssid);
                }
                
                Serial.println("  Pushing to vector...");
                networks.push_back(info);
                Serial.println("  Push complete");
            }
            
            Serial.println("[WiFi] Starting bubble sort");
            // Simple bubble sort - safer than std::sort on ESP32
            for (int i = 0; i < (int)networks.size() - 1; i++) {
                for (int j = 0; j < (int)networks.size() - i - 1; j++) {
                    if (networks[j].rssi < networks[j + 1].rssi) {
                        NetworkInfo temp = networks[j];
                        networks[j] = networks[j + 1];
                        networks[j + 1] = temp;
                    }
                }
            }
            Serial.println("[WiFi] Sort complete");
            
            // Free WiFi scan data from memory
            WiFi.scanDelete();
            Serial.println("[WiFi] Scan data freed");
            
            // Release processing lock and mark data ready
            processingData = false;
            scanDataReady = true;
            Serial.println("[WiFi] Loop processing complete - data ready");
        }
    }
}

void WiFiModule::drawMenu(DisplayManager* display) {
    Serial.println("[WiFi] drawMenu called");
    this->displayManager = display; // Store for later use if needed
    // display->clearContent();

    if (currentState == MENU) {
        Serial.println("[WiFi] Drawing MENU state");
        display->clearMenu();
        display->drawMenuTitle("WiFi Tools");
        display->drawMenuItem(isScanning ? "Stop Scan" : "Start Scan", 0, selectedIndex == 0);
        for (int i = 0; i < menuItemsCount; i++) {
            
            display->drawMenuItem(menuItems[i], i+1, selectedIndex == i+1);
        }
        display->drawScrollBar(menuItemsCount + 1, scrollOffset, 3);
        display->getTFT()->setTextColor(TFT_WHITE);
        display->getTFT()->setTextDatum(BL_DATUM);
        display->getTFT()->drawString(isScanning ? "Scanning..." : "Idle", 310, 230, 2);
        display->updateMenu();
    } else {
        display->clearContent();
        // Draw sub-menu or tool interface based on currentState
        switch (currentState) {
            case SCAN_RESULTS:
                Serial.println("[WiFi] Drawing SCAN_RESULTS");
                display->drawMenuTitle("Scan Results");
                
                // Don't display if data is being processed
                if (processingData) {
                    Serial.println("[WiFi] Data being processed, showing wait message");
                    display->getTFT()->setTextColor(TFT_YELLOW);
                    display->getTFT()->setTextDatum(MC_DATUM);
                    display->getTFT()->drawString("Processing...", 160, 80, 2);
                    return;
                }
                
                if (networks.empty() && isScanning) {
                    Serial.println("[WiFi] Showing scanning message - no data yet");
                    display->getTFT()->setTextColor(TFT_YELLOW);
                    display->getTFT()->setTextDatum(MC_DATUM);
                    display->getTFT()->drawString("Scanning WiFi...", 160, 80, 2);
                    display->getTFT()->setTextColor(TFT_WHITE);
                    display->getTFT()->drawString("Please wait", 160, 110, 2);
                } else if (networks.empty() && !isScanning) {
                    Serial.println("[WiFi] No networks found");
                    display->getTFT()->setTextColor(TFT_RED);
                    display->getTFT()->setTextDatum(MC_DATUM);
                    display->getTFT()->drawString("No Networks Found", 160, 80, 2);
                    display->getTFT()->setTextColor(TFT_WHITE);
                    display->getTFT()->drawString("Press Back", 160, 110, 2);
                } else {
                    Serial.print("[WiFi] Drawing results, count: ");
                    Serial.println(networks.size());
                    
                    display->clearMenu();
                    
                    int count = networks.size();
                    
                    // Safety check
                    if (count == 0) {
                        Serial.println("[WiFi] ERROR: Count is 0 after check!");
                        return;
                    }
                    
                    Serial.println("[WiFi] Drawing simple network list");
                    
                    // Draw up to 8 networks
                    int maxDisplay = (count > 8) ? 8 : count;
                    int y = 40;
                    
                    display->getTFT()->setTextDatum(TL_DATUM);
                    display->getTFT()->setTextColor(TFT_WHITE);
                    
                    for (int i = 0; i < maxDisplay; i++) {
                        // Copy to avoid any vector access issues
                        String ssid = networks[i].ssid;
                        int rssi = networks[i].rssi;
                        
                        // SSID - use direct drawString with const char*
                        display->getTFT()->drawString(ssid.c_str(), 5, y, 2);
                        
                        // RSSI
                        display->getTFT()->setTextDatum(TR_DATUM);
                        display->getTFT()->drawNumber(rssi, 315, y, 2);
                        display->getTFT()->setTextDatum(TL_DATUM);
                        
                        y += 22;
                    }
                    
                    // Simple status
                    display->getTFT()->setTextColor(TFT_WHITE);
                    display->getTFT()->setTextDatum(MC_DATUM);
                    display->getTFT()->drawNumber(count, 160, 230, 1);
                    
                    Serial.println("[WiFi] SCAN_RESULTS drawing complete");
                }
                break;
            case EVIL_PORTAL:
                display->drawMenuTitle("Evil Portal");
                display->getTFT()->setTextColor(TFT_WHITE);
                display->getTFT()->setTextDatum(TL_DATUM);
                display->getTFT()->drawString("Functionality TBD EVIL PORTAL", 10, 40, 2);
                break;
            case KARMA_ATTACK:
                display->drawMenuTitle("Karma Attack");
                display->getTFT()->setTextColor(TFT_WHITE);
                display->getTFT()->setTextDatum(TL_DATUM);
                display->getTFT()->drawString("Functionality TBD KARMA ATTACK", 10, 40, 2);
                break;
            case RESPONDER:
                display->drawMenuTitle("Responder");
                display->getTFT()->setTextColor(TFT_WHITE);
                display->getTFT()->setTextDatum(TL_DATUM);
                display->getTFT()->drawString("Functionality TBD RESPONDER", 10, 40, 2);
                break;
            case SNIFFER:
                display->drawMenuTitle("Sniffer");
                display->getTFT()->setTextColor(TFT_WHITE);
                display->getTFT()->setTextDatum(TL_DATUM);
                display->getTFT()->drawString("Functionality TBD SNIFFER", 10, 40, 2);
                break;
            case DEAUTH_ATTACK:
                display->drawMenuTitle("Deauth Attack");
                display->getTFT()->setTextColor(TFT_WHITE);
                display->getTFT()->setTextDatum(TL_DATUM);
                display->getTFT()->drawString("Functionality TBD DEAUTH ATTACK", 10, 40, 2);
                break;
            case CAPTURE_HANDSHAKE:
                display->drawMenuTitle("Capture Handshake");
                display->getTFT()->setTextColor(TFT_WHITE);
                display->getTFT()->setTextDatum(TL_DATUM);
                display->getTFT()->drawString("Functionality TBD CAPTURE HANDSHAKE", 10, 40, 2);
                break;
            default:
                break;
        }
    }
}

bool WiFiModule::handleInput(uint8_t button) {
    Serial.print("[WiFi] handleInput: button=");
    Serial.print(button);
    Serial.print(" state=");
    Serial.println(currentState);
    
    // Safety check
    if (!displayManager) {
        Serial.println("[WiFi] ERROR: displayManager is null!");
        return true;
    }
    
    // 0=Up, 1=Down, 2=Select, 3=Back
    if (currentState == MENU) {
        if (button == 0) { // Up
            selectedIndex--;
            if (selectedIndex < 0) {
                selectedIndex = menuItemsCount; // Wrap around
            }
            // Adjust scroll offset
            if (selectedIndex < scrollOffset) {
                scrollOffset = selectedIndex;
            } else if (selectedIndex >= scrollOffset + 3) {
                scrollOffset = selectedIndex - 2;
            }
            drawMenu(displayManager); // Force redraw
        } else if (button == 1) { // Down
            selectedIndex++;
            if (selectedIndex > menuItemsCount) {
                selectedIndex = 0; // Wrap around
            }
            // Adjust scroll offset
            if (selectedIndex >= scrollOffset + 3) {
                scrollOffset = selectedIndex - 2;
            } else if (selectedIndex < scrollOffset) {
                scrollOffset = selectedIndex;
            }
            drawMenu(displayManager); // Force redraw
        } else if (button == 2) { // Select
            if (selectedIndex == 0) { // Start/Stop Scan
                if (isScanning) {
                    Serial.println("[WiFi] Stopping scan");
                    // Stop scan
                    isScanning = false;
                    WiFi.scanDelete();
                } else {
                    Serial.println("[WiFi] Starting WiFi scan");
                    // Start scan (stay in menu, don't switch to results)
                    isScanning = true;
                    scanComplete = false;
                    scanDataReady = false;
                    networks.clear();
                    Serial.println("[WiFi] Calling WiFi.scanNetworks(true)");
                    WiFi.scanNetworks(true); // Async scan
                    Serial.println("[WiFi] Scan started in background");
                }
                drawMenu(displayManager); // Force redraw
                return true;
            } else {
                // Map menu index to correct state
                // menuItems: Scan Results=1, Evil Portal=2, Karma=3, Responder=4, Sniffer=5, Deauth=6, Capture=7
                Serial.print("[WiFi] Selected menu item: ");
                Serial.println(selectedIndex);
                currentState = static_cast<State>(selectedIndex); // selectedIndex 1->SCAN_RESULTS, 2->EVIL_PORTAL, etc.
            }
            drawMenu(displayManager); // Force redraw
        } else if (button == 3) { // Back
            return false; // Exit module
        }
    } else if (currentState == SCAN_RESULTS) {
        if (button == 3) { // Back
            if (isScanning) {
                WiFi.scanDelete();
                isScanning = false;
            }
            currentState = MENU;
            drawMenu(displayManager);
        }
        // Removed up/down/select - no selection in simple mode
    } else {
        if (button == 3) { // Back
            currentState = MENU;
            drawMenu(displayManager);
        }
    }
    return true; // Continue in module
}

String WiFiModule::getEncryptionType(wifi_auth_mode_t encryption) {
    switch (encryption) {
        case WIFI_AUTH_OPEN: return "Open";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-E";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/3";
        default: return "Unknown";
    }
}

String WiFiModule::getSignalStrength(int32_t rssi) {
    if (rssi >= -50) return "[####]";
    if (rssi >= -60) return "[### ]";
    if (rssi >= -70) return "[##  ]";
    if (rssi >= -80) return "[#   ]";
    return "[    ]";
}
