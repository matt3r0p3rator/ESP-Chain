#include "wifi_module.h"
#include <algorithm>

// ======================================================================================
// WiFi Module Implementation
// ======================================================================================

void WiFiModule::init() {
    currentState = MENU;
    menuIndex = 0;
    selectedIndex = 0;
    settingsIndex = 0;
    isScanning = false;
    lastScanTime = 0;
    
    // Initialize WiFi in Station mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_log_level_set("wifi", ESP_LOG_NONE);

    // Initialize Packet Queue for captures
    if (packetQueue == nullptr) {
        packetQueue = xQueueCreate(10, sizeof(CapturedPacket));
    }
}

void WiFiModule::loop() {
    extern DisplayManager displayManager;

    // 1. Process any captured packets (for handshake capture)
    processPacketQueue();

    // 2. Handle Deauth Attack
    if (isDeauthing) {
        sendDeauthFrame();
        delay(10); // Yield to prevent watchdog
    }

    // 3. Update Attack UI (Live stats)
    if (currentState == ATTACK_DEAUTH || currentState == HANDSHAKE_CAPTURE || 
        currentState == ATTACK_MIXED || currentState == STATION_SCAN) {
        static unsigned long lastDraw = 0;
        if (millis() - lastDraw > 200) {
            updateUI(&displayManager);
            lastDraw = millis();
        }
    }

    // 4. Handle Scanning Logic
    if (isScanning) {
        int n = WiFi.scanComplete();
        
        if (n == -2) {
            // Scan not started, start it
            WiFi.scanNetworks(true, showHidden, false, scanTimePerChannel);
        } else if (n == -1) {
            // Scan in progress, do nothing
        } else if (n >= 0) {
            // Scan completed
            processScanResults(n);
            WiFi.scanDelete();
            
            // Restart scan immediately for continuous updates
            WiFi.scanNetworks(true, showHidden, false, scanTimePerChannel);
            
            // If we are viewing results, refresh the display
            if (currentState == RESULTS) {
                drawMenu(&displayManager);
            }
        }
    }
}

void WiFiModule::processScanResults(int networksFound) {
    unsigned long now = millis();
    
    // 1. Update existing networks and add new ones
    for (int i = 0; i < networksFound; ++i) {
        String ssid = WiFi.SSID(i);
        if (ssid.isEmpty() && !showHidden) continue;
        if (ssid.isEmpty()) ssid = "<HIDDEN>";
        
        String bssid = WiFi.BSSIDstr(i);
        int32_t rssi = WiFi.RSSI(i);
        uint8_t channel = WiFi.channel(i);
        wifi_auth_mode_t encryption = WiFi.encryptionType(i);
        
        bool found = false;
        for (auto& network : scanResults) {
            if (network.bssid == bssid) {
                // Update existing
                network.ssid = ssid;
                network.rssi = rssi;
                network.channel = channel;
                network.encryption = encryption;
                network.lastSeen = now;
                found = true;
                break;
            }
        }
        
        if (!found) {
            // Add new
            APInfo newAP;
            newAP.ssid = ssid;
            newAP.bssid = bssid;
            newAP.rssi = rssi;
            newAP.channel = channel;
            newAP.encryption = encryption;
            newAP.lastSeen = now;
            scanResults.push_back(newAP);
        }
    }
    
    // 2. Remove old networks (not seen in last 20 seconds)
    // Using 20s to be very generous and prevent flickering
    for (auto it = scanResults.begin(); it != scanResults.end(); ) {
        if (now - it->lastSeen > 20000) {
            it = scanResults.erase(it);
        } else {
            ++it;
        }
    }
    
    // 3. Sort the list
    sortResults();
}

void WiFiModule::sortResults() {
    // Stable sort to prevent jumping
    if (sortMethod == SORT_RSSI) {
        std::sort(scanResults.begin(), scanResults.end(), [](const APInfo& a, const APInfo& b) {
            if (a.rssi != b.rssi) return a.rssi > b.rssi; // Higher RSSI first
            return a.ssid < b.ssid; // Alphabetical tie-breaker
        });
    } else {
        std::sort(scanResults.begin(), scanResults.end(), [](const APInfo& a, const APInfo& b) {
            if (a.channel != b.channel) return a.channel < b.channel; // Lower channel first
            return a.rssi > b.rssi; // RSSI tie-breaker
        });
    }
}

String WiFiModule::getName() {
    return "WiFi Tools";
}

const unsigned char* WiFiModule::getIcon() { return image_wifi_bits; }
int WiFiModule::getIconWidth() { return 19; }
int WiFiModule::getIconHeight() { return 16; }
int WiFiModule::getIconOffsetY() { return 0; }
int WiFiModule::getIconSpacing() { return 13; }

String WiFiModule::getDescription() {
    return "Scanner & Attacks";
}

void WiFiModule::drawMenu(DisplayManager* display) {
    display->clearContent();

    switch (currentState) {
        case MENU:
            display->drawMenuTitle("WiFi Menu");
            display->drawMenuItem("Scanner", 0, menuIndex == 0);
            display->drawMenuItem("Settings", 1, menuIndex == 1);
            break;

        case SCANNER_MENU:
            display->drawMenuTitle("WiFi Scanner");
            display->drawMenuItem(isScanning ? "Stop Scan" : "Start Scan", 0, menuIndex == 0);
            display->drawMenuItem("View Results", 1, menuIndex == 1);
            
            if (isScanning) {
                display->getTFT()->setTextDatum(MC_DATUM);
                display->getTFT()->setTextColor(TFT_GREEN, THEME_BG);
                display->getTFT()->drawString("Scanning...", 160, 160, 2);
            }
            break;

        case SETTINGS:
            display->drawMenuTitle("Settings");
            for (int i = 0; i < 3; i++) {
                String label = settingsItems[i];
                if (i == 0) label += ": " + String(scanTimePerChannel) + "ms";
                if (i == 1) label += ": " + String(showHidden ? "ON" : "OFF");
                if (i == 2) label += ": " + String(sortMethod == SORT_RSSI ? "RSSI" : "CH");
                display->drawMenuItem(label, i, i == settingsIndex);
            }
            break;

        case SETTINGS_SCAN_TIME:
            display->drawMenuTitle("Scan Time");
            display->getTFT()->setTextDatum(MC_DATUM);
            display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
            display->getTFT()->drawString(String(scanTimePerChannel) + " ms", 160, 100, 4);
            display->getTFT()->drawString("Single: +100ms", 160, 140, 2);
            display->getTFT()->drawString("Double: Save", 160, 160, 2);
            break;

        case RESULTS:
            if (scanResults.empty()) {
                display->drawMenuTitle("Results (0)");
                display->getTFT()->setTextDatum(MC_DATUM);
                display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
                display->getTFT()->drawString("No Networks Found", 160, 100, 2);
                if (isScanning) {
                    display->getTFT()->drawString("Scanning...", 160, 130, 2);
                }
            } else {
                display->drawMenuTitle("Results (" + String(scanResults.size()) + ")");
                
                // Calculate visible range
                int itemsPerPage = 5;
                int start = 0;
                
                // Keep selected item in view
                if (selectedIndex >= itemsPerPage) {
                    start = selectedIndex - (itemsPerPage - 1);
                }
                if (start < 0) start = 0;
                
                // Draw items
                for (int i = 0; i < itemsPerPage; i++) {
                    int idx = start + i;
                    if (idx >= scanResults.size()) break;
                    
                    APInfo& ap = scanResults[idx];
                    String label = ap.ssid;
                    if (label.length() > 14) label = label.substring(0, 14) + "..";
                    label += " (" + String(ap.rssi) + ")";
                    
                    display->drawMenuItem(label, i, idx == selectedIndex);
                }
            }
            break;

        case DETAILS:
            display->drawMenuTitle("Network Details");
            display->getTFT()->setTextDatum(TL_DATUM);
            display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
            
            display->getTFT()->drawString("SSID: " + selectedTarget.ssid, 10, 30, 2);
            display->getTFT()->drawString("BSSID: " + selectedTarget.bssid, 10, 50, 2);
            display->getTFT()->drawString("CH: " + String(selectedTarget.channel), 10, 70, 2);
            display->getTFT()->drawString("RSSI: " + String(selectedTarget.rssi), 10, 90, 2);
            display->getTFT()->drawString("Enc: " + getEncryptionName(selectedTarget.encryption), 10, 110, 2);
            
            display->getTFT()->setTextDatum(MC_DATUM);
            display->getTFT()->drawString("Double Click: Options", 160, 150, 2);
            break;

        case TARGET_OPTIONS:
            display->drawMenuTitle(selectedTarget.ssid);
            display->drawMenuItem("Deauth Attack", 0, menuIndex == 0); 
            display->drawMenuItem("Capture Handshake", 1, menuIndex == 1);
            display->drawMenuItem("Mixed Attack", 2, menuIndex == 2);
            display->drawMenuItem("Scan Clients", 3, menuIndex == 3);
            break;

        case STATION_SCAN:
            display->drawMenuTitle("Scanning Clients...");
            display->getTFT()->setTextDatum(MC_DATUM);
            display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
            display->getTFT()->drawString("Found: " + String(detectedStations.size()), 160, 100, 4);
            display->getTFT()->drawString("Double Click to List", 160, 140, 2);
            display->getTFT()->fillCircle(160, 180, 5, (millis() / 500) % 2 == 0 ? TFT_GREEN : TFT_BLACK);
            break;

        case STATION_LIST:
            if (detectedStations.empty()) {
                display->drawMenuTitle("No Clients Found");
                display->getTFT()->setTextDatum(MC_DATUM);
                display->getTFT()->drawString("Go Back to Scan", 160, 100, 2);
            } else {
                display->drawMenuTitle("Select Client (" + String(detectedStations.size()) + ")");
                int start = 0;
                if (stationListIndex > 2) start = stationListIndex - 2;
                if (start + 5 > (int)detectedStations.size()) start = detectedStations.size() - 5;
                if (start < 0) start = 0;

                for (int i = 0; i < 5 && (start + i) < (int)detectedStations.size(); i++) {
                    int idx = start + i;
                    String label = detectedStations[idx];
                    if (label == selectedStation) label = "> " + label;
                    display->drawMenuItem(label, i, idx == stationListIndex);
                }
            }
            break;

        case ATTACK_DEAUTH:
        case HANDSHAKE_CAPTURE:
        case ATTACK_MIXED:
            drawTerminal(display);
            break;
    }
}

bool WiFiModule::handleInput(uint8_t button) {
    extern DisplayManager displayManager;

    // Button 3: Back / Long Press
    if (button == 3) {
        switch (currentState) {
            case MENU:
                return false; // Exit module
            case SCANNER_MENU:
                currentState = MENU;
                break;
            case SETTINGS:
                currentState = MENU;
                break;
            case SETTINGS_SCAN_TIME:
                currentState = SETTINGS;
                break;
            case RESULTS:
                currentState = SCANNER_MENU;
                break;
            case DETAILS:
                currentState = RESULTS;
                break;
            case TARGET_OPTIONS:
                currentState = DETAILS;
                break;
            case ATTACK_DEAUTH:
                stopDeauth();
                currentState = TARGET_OPTIONS;
                break;
            case HANDSHAKE_CAPTURE:
                stopHandshakeCapture();
                currentState = TARGET_OPTIONS;
                break;
            case ATTACK_MIXED:
                stopMixedAttack();
                currentState = TARGET_OPTIONS;
                break;
            case STATION_SCAN:
                stopStationScan();
                currentState = TARGET_OPTIONS;
                break;
            case STATION_LIST:
                currentState = STATION_SCAN;
                break;
            default:
                currentState = MENU;
                break;
        }
        drawMenu(&displayManager);
        return true;
    }

    // Button 0: Scroll Up
    if (button == 0) {
        switch (currentState) {
            case MENU:
            case SCANNER_MENU:
                menuIndex = (menuIndex > 0) ? menuIndex - 1 : 1;
                break;
            case SETTINGS:
                settingsIndex = (settingsIndex > 0) ? settingsIndex - 1 : 2;
                break;
            case SETTINGS_SCAN_TIME:
                if (scanTimePerChannel > 100) scanTimePerChannel -= 100;
                break;
            case RESULTS:
                if (!scanResults.empty()) {
                    selectedIndex = (selectedIndex > 0) ? selectedIndex - 1 : scanResults.size() - 1;
                }
                break;
            case TARGET_OPTIONS:
                menuIndex = (menuIndex > 0) ? menuIndex - 1 : 3;
                break;
            case STATION_LIST:
                if (!detectedStations.empty()) {
                    stationListIndex = (stationListIndex > 0) ? stationListIndex - 1 : detectedStations.size() - 1;
                }
                break;
            default:
                break;
        }
        drawMenu(&displayManager);
        return true;
    }

    // Button 1: Scroll Down (Single Click)
    if (button == 1) {
        switch (currentState) {
            case MENU:
            case SCANNER_MENU:
                menuIndex = (menuIndex + 1) % 2;
                break;
            case SETTINGS:
                settingsIndex = (settingsIndex + 1) % 3;
                break;
            case SETTINGS_SCAN_TIME:
                scanTimePerChannel += 100;
                if (scanTimePerChannel > 1000) scanTimePerChannel = 100;
                break;
            case RESULTS:
                if (!scanResults.empty()) {
                    selectedIndex = (selectedIndex + 1) % scanResults.size();
                }
                break;
            case TARGET_OPTIONS:
                menuIndex = (menuIndex + 1) % 4;
                break;
            case STATION_LIST:
                if (!detectedStations.empty()) {
                    stationListIndex = (stationListIndex + 1) % detectedStations.size();
                }
                break;
            default:
                break;
        }
        drawMenu(&displayManager);
        return true;
    }

    // Button 2: Select (Double Click)
    if (button == 2) {
        switch (currentState) {
            case MENU:
                if (menuIndex == 0) {
                    currentState = SCANNER_MENU;
                    menuIndex = 0;
                } else {
                    currentState = SETTINGS;
                }
                break;
            case SCANNER_MENU:
                if (menuIndex == 0) {
                    isScanning = !isScanning;
                    if (!isScanning) {
                        WiFi.scanDelete();
                    }
                } else {
                    currentState = RESULTS;
                    selectedIndex = 0;
                }
                break;
            case SETTINGS:
                if (settingsIndex == 0) currentState = SETTINGS_SCAN_TIME;
                else if (settingsIndex == 1) showHidden = !showHidden;
                else if (settingsIndex == 2) sortMethod = (sortMethod == SORT_RSSI) ? SORT_CHANNEL : SORT_RSSI;
                break;
            case SETTINGS_SCAN_TIME:
                currentState = SETTINGS;
                break;
            case RESULTS:
                if (!scanResults.empty()) {
                    selectedTarget = scanResults[selectedIndex];
                    currentState = DETAILS;
                }
                break;
            case DETAILS:
                currentState = TARGET_OPTIONS;
                menuIndex = 0;
                selectedStation = "";
                break;
            case TARGET_OPTIONS:
                if (menuIndex == 0) {
                    startDeauth();
                    currentState = ATTACK_DEAUTH;
                } else if (menuIndex == 1) {
                    startHandshakeCapture();
                    currentState = HANDSHAKE_CAPTURE;
                } else if (menuIndex == 2) {
                    startMixedAttack();
                    currentState = ATTACK_MIXED;
                } else if (menuIndex == 3) {
                    startStationScan();
                    currentState = STATION_SCAN;
                }
                break;
            case STATION_SCAN:
                currentState = STATION_LIST;
                stationListIndex = 0;
                break;
            case STATION_LIST:
                if (!detectedStations.empty()) {
                    selectedStation = detectedStations[stationListIndex];
                    currentState = TARGET_OPTIONS;
                }
                break;
            default:
                break;
        }
        drawMenu(&displayManager);
        return true;
    }

    return true;
}

void WiFiModule::updateUI(DisplayManager* display) {
    switch (currentState) {
        case ATTACK_DEAUTH:
        case HANDSHAKE_CAPTURE:
        case ATTACK_MIXED:
            drawTerminalUpdate(display);
            break;
            
        case STATION_SCAN:
            display->getTFT()->setTextDatum(MC_DATUM);
            display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
            display->getTFT()->drawString("Found: " + String(detectedStations.size()), 160, 100, 4);
            display->getTFT()->fillCircle(160, 180, 5, (millis() / 500) % 2 == 0 ? TFT_GREEN : TFT_BLACK);
            break;
            
        default:
            break;
    }
}

String WiFiModule::getEncryptionName(wifi_auth_mode_t encryption) {
    switch (encryption) {
        case WIFI_AUTH_OPEN: return "Open";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-E";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
        case WIFI_AUTH_WAPI_PSK: return "WAPI";
        default: return "Unknown";
    }
}
