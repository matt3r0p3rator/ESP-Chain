#include "wifi_module.h"
#include <algorithm>

void WiFiModule::init() {
    currentState = MENU;
    menuIndex = 0;
    selectedIndex = 0;
    settingsIndex = 0;
    isScanning = false;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_log_level_set("wifi", ESP_LOG_NONE);
}

void WiFiModule::loop() {
    extern DisplayManager displayManager;

    if (isDeauthing) {
        sendDeauthFrame();
        delay(10); // Prevent watchdog trigger and allow other tasks
    }

    // Live update for attack screens
    if (currentState == ATTACK_DEAUTH || currentState == HANDSHAKE_CAPTURE || currentState == ATTACK_MIXED || currentState == STATION_SCAN) {
        static unsigned long lastDraw = 0;
        if (millis() - lastDraw > 200) {
            updateUI(&displayManager);
            lastDraw = millis();
        }
    }

    if (isScanning) {
        int n = WiFi.scanComplete();
        if (n == -2) {
            // Start scan
            WiFi.scanNetworks(true, showHidden, false, scanTimePerChannel);
        } else if (n >= 0) {
            // Scan done, process results
            scanResults.clear();
            for (int i = 0; i < n; ++i) {
                APInfo ap;
                ap.ssid = WiFi.SSID(i);
                if (ap.ssid.isEmpty() && !showHidden) continue; 
                if (ap.ssid.isEmpty()) ap.ssid = "<HIDDEN>";
                
                ap.rssi = WiFi.RSSI(i);
                ap.channel = WiFi.channel(i);
                ap.bssid = WiFi.BSSIDstr(i);
                ap.encryption = WiFi.encryptionType(i);
                scanResults.push_back(ap);
            }
            sortResults();
            WiFi.scanDelete();
            
            // Restart scan immediately
            WiFi.scanNetworks(true, showHidden, false, scanTimePerChannel);
        }
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
    return "WiFi Tools";
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
            
            // Show status
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

        case SETTINGS_SHOW_HIDDEN:
            break;

        case SETTINGS_SORT_METHOD:
            break;

        case RESULTS:
            if (scanResults.empty()) {
                display->getTFT()->setTextDatum(MC_DATUM);
                display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
                display->getTFT()->drawString("No Networks Found", 160, 100, 2);
                if (isScanning) {
                    display->getTFT()->drawString("Scanning...", 160, 130, 2);
                }
            } else {
                display->drawMenuTitle("Results (" + String(scanResults.size()) + ")");
                // Show 5 items centered around selectedIndex
                int start = 0;
                if (selectedIndex > 2) start = selectedIndex - 2;
                if (start + 5 > (int)scanResults.size()) start = scanResults.size() - 5;
                if (start < 0) start = 0;

                for (int i = 0; i < 5 && (start + i) < (int)scanResults.size(); i++) {
                    int idx = start + i;
                    String label = scanResults[idx].ssid;
                    if (label.length() > 14) label = label.substring(0, 14) + "..";
                    label += " (" + String(scanResults[idx].rssi) + ")";
                    display->drawMenuItem(label, i, idx == selectedIndex);
                }
            }
            break;

        case DETAILS:
            display->drawMenuTitle("Network Details");
            display->getTFT()->setTextDatum(TL_DATUM);
            display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
            
            // Moved up by 10 pixels
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
            drawTerminal(display);
            break;

        case HANDSHAKE_CAPTURE:
            drawTerminal(display);
            break;

        case ATTACK_MIXED:
            drawTerminal(display);
            break;
    }
}

bool WiFiModule::handleInput(uint8_t button) {
    extern DisplayManager displayManager;

    if (button == 3) { // Back / Long Press
        switch (currentState) {
            case MENU:
                return false; 
            case SCANNER_MENU:
                currentState = MENU;
                // Do NOT stop scanning here, user wants background scan
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
                currentState = STATION_SCAN; // Back to scanning
                break;
            default:
                currentState = MENU;
                break;
        }
        drawMenu(&displayManager);
        return true;
    }

    if (button == 1) { // Scroll (Single Click)
        switch (currentState) {
            case MENU:
                menuIndex = (menuIndex + 1) % 2; 
                break;
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
            case DETAILS:
                // Maybe scroll details if too long? For now nothing.
                break;
            case TARGET_OPTIONS:
                menuIndex = (menuIndex + 1) % 4;
                break;
            case ATTACK_DEAUTH:
                // Do nothing
                break;
            case HANDSHAKE_CAPTURE:
                // Do nothing
                break;
            case ATTACK_MIXED:
                // Do nothing
                break;
            case STATION_SCAN:
                // Do nothing
                break;
            case STATION_LIST:
                if (!detectedStations.empty()) {
                    stationListIndex = (stationListIndex + 1) % detectedStations.size();
                }
                break;
        }
        drawMenu(&displayManager);
        return true;
    }

    if (button == 2) { // Select (Double Click)
        switch (currentState) {
            case MENU:
                if (menuIndex == 0) { // Scanner
                    currentState = SCANNER_MENU;
                    menuIndex = 0;
                } else if (menuIndex == 1) { // Settings
                    currentState = SETTINGS;
                }
                break;
            case SCANNER_MENU:
                if (menuIndex == 0) { // Start/Stop Scan
                    isScanning = !isScanning;
                    if (!isScanning) {
                        // Stop scan if possible?
                        // WiFi.scanDelete(); // Maybe?
                    }
                } else if (menuIndex == 1) { // View Results
                    currentState = RESULTS;
                    selectedIndex = 0;
                }
                break;
            case SETTINGS:
                if (settingsIndex == 0) {
                    currentState = SETTINGS_SCAN_TIME;
                } else if (settingsIndex == 1) {
                    showHidden = !showHidden;
                } else if (settingsIndex == 2) {
                    sortMethod = (sortMethod == SORT_RSSI) ? SORT_CHANNEL : SORT_RSSI;
                }
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
                selectedStation = ""; // Reset selected station
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
            case ATTACK_DEAUTH:
                // Maybe toggle?
                break;
            case HANDSHAKE_CAPTURE:
                break;
            case ATTACK_MIXED:
                break;
            case STATION_SCAN:
                currentState = STATION_LIST;
                stationListIndex = 0;
                break;
            case STATION_LIST:
                if (!detectedStations.empty()) {
                    selectedStation = detectedStations[stationListIndex];
                    currentState = TARGET_OPTIONS; // Go back to options with station selected
                }
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

void WiFiModule::performScan() {
    // Deprecated, using async scan in loop
}

void WiFiModule::sortResults() {
    if (sortMethod == SORT_RSSI) {
        std::sort(scanResults.begin(), scanResults.end(), [](const APInfo& a, const APInfo& b) {
            return a.rssi > b.rssi; // Descending RSSI
        });
    } else {
        std::sort(scanResults.begin(), scanResults.end(), [](const APInfo& a, const APInfo& b) {
            return a.channel < b.channel; // Ascending Channel
        });
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
