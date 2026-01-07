#include "wifi_module.h"
#include "WiFi.h"
#include "config_manager.h"
#include "display_manager.h"
#include "../../ui/icons.h"
#include <SD.h>

extern DisplayManager displayManager;

String WiFiModule::getEncryptionType(wifi_auth_mode_t encryption) {
    switch (encryption) {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-E";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/3";
        case WIFI_AUTH_WAPI_PSK: return "WAPI";
        default: return "UNKN";
    }
}

String WiFiModule::getSignalStrength(int32_t rssi) {
    if (rssi >= -50) return "Excellent";
    if (rssi >= -60) return "Good";
    if (rssi >= -70) return "Fair";
    if (rssi >= -80) return "Weak";
    return "Very Weak";
}

void WiFiModule::init() {
    currentState = MENU;
    menuIndex = 0;
    selectedIndex = -1;
    scrollOffset = 0;
    isScanning = false;
}

void WiFiModule::loop() {
    if (isScanning) {
        // Check if scan is complete
        int n = WiFi.scanComplete();
        if (n >= 0) {
            // Scan finished
            networks.clear();
            networkCount = n;
            
            for (int i = 0; i < n; i++) {
                NetworkInfo info;
                info.ssid = WiFi.SSID(i);
                info.rssi = WiFi.RSSI(i);
                info.encryption = WiFi.encryptionType(i);
                info.channel = WiFi.channel(i);
                networks.push_back(info);
            }
            
            // Start next scan
            WiFi.scanDelete();
            WiFi.scanNetworks(true); // Async scan
        } else if (n == WIFI_SCAN_FAILED) {
            // Scan failed, restart
            WiFi.scanDelete();
            WiFi.scanNetworks(true);
        }
    }
}

void WiFiModule::drawMenu(DisplayManager* display) {
    if (!display || !display->getTFT()) return;
    
    this->displayManager = display;

    if (currentState == MENU) {
        display->clearMenu();
        display->drawMenuTitle("WiFi Tools");
        display->drawMenuItem(isScanning ? "Stop Scan --- Scanning..." : "Start Scan", 0, menuIndex == 0);
        display->drawMenuItem("View Scan Results", 1, menuIndex == 1);
        display->updateMenu();
    }
    else if (currentState == VIEW_SCAN) {
        display->clearMenu();
        
        if (networks.empty()) {
            display->getTFT()->setTextDatum(TL_DATUM);
            display->getTFT()->setTextColor(TFT_YELLOW, TFT_BLACK);
            display->getTFT()->drawString("No networks found", 10, 40, 2);
            display->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
            display->getTFT()->drawString("Start a scan first", 10, 65, 2);
            display->getTFT()->drawString("BACK: Return to Menu", 10, 220, 2);
        } else {
            int itemsPerPage = 5;
            int start = scrollOffset;
            int end = start + itemsPerPage;
            if (end > networks.size()) end = networks.size();
            
            for (int i = start; i < end; i++) {
                // Format: "SSID | -70dBm | WPA2 | Ch:6"
                String menuText = networks[i].ssid;
                if (menuText.length() > 15) menuText = menuText.substring(0, 15);
                menuText += " " + String(networks[i].rssi) + "dBm";
                
                display->drawMenuItem(menuText, i - start, i == selectedIndex);
            }
            
            // Draw scroll bar if needed
            if (networks.size() > itemsPerPage) {
                display->drawScrollBar(networks.size(), scrollOffset, itemsPerPage);
            }
            
            display->getTFT()->setTextDatum(TL_DATUM);
            display->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
            display->getTFT()->drawString("BACK: Return", 10, 220, 2);
        }
        
        display->updateMenu();
    }
}

bool WiFiModule::handleInput(uint8_t button) {
    // 0=Up, 1=Down, 2=Select, 3=Back

    if (!displayManager) return true;

    if (currentState == MENU) {
        if (button == 0) { // Up
            menuIndex--;
            if (menuIndex < 0) menuIndex = 1;
            drawMenu(displayManager); // Force redraw
        } else if (button == 1) { // Down
            menuIndex++;
            if (menuIndex > 1) menuIndex = 0;
            drawMenu(displayManager); // Force redraw
        } else if (button == 2) { // Select
            if (menuIndex == 0) {
                // Start/Stop Scan
                if (!isScanning) {
                    // Start Scan
                    WiFi.mode(WIFI_STA);
                    WiFi.disconnect();
                    networks.clear();
                    networkCount = 0;
                    WiFi.scanNetworks(true); // Start async scan
                    isScanning = true;
                } else {
                    // Stop Scan
                    WiFi.scanDelete();
                    isScanning = false;
                }
                drawMenu(displayManager); // Force redraw
            } else if (menuIndex == 1) {
                currentState = VIEW_SCAN;
                scrollOffset = 0;
                selectedIndex = networks.empty() ? -1 : 0;
                drawMenu(displayManager); // Force redraw
            }
        } else if (button == 3) { // Back
            // Stop any scanning when exiting module
            if (isScanning) {
                WiFi.scanDelete();
                isScanning = false;
            }
            return false; // Exit module
        }
    }
    else if (currentState == VIEW_SCAN) {
        int itemsPerPage = 5;
        if (button == 0) { // Up
            if (selectedIndex > 0) {
                selectedIndex--;
                if (selectedIndex < scrollOffset) {
                    scrollOffset--;
                }
            } else if (selectedIndex == 0) {
                // Wrap to bottom
                selectedIndex = networks.size() - 1;
                scrollOffset = max(0, (int)networks.size() - itemsPerPage);
            }
            drawMenu(displayManager);
        } else if (button == 1) { // Down
            if (selectedIndex < networks.size() - 1) {
                selectedIndex++;
                if (selectedIndex >= scrollOffset + itemsPerPage) {
                    scrollOffset++;
                }
            } else if (selectedIndex == networks.size() - 1) {
                // Wrap to top
                selectedIndex = 0;
                scrollOffset = 0;
            }
            drawMenu(displayManager);
        } else if (button == 2) { // Select
            // Could add network details view here
            if (selectedIndex >= 0 && selectedIndex < networks.size()) {
                // Show network details or perform action
            }
        } else if (button == 3) { // Back
            currentState = MENU;
            menuIndex = 0;
            scrollOffset = 0;
            selectedIndex = -1;
            drawMenu(displayManager);
        }
    }

    return true;
}