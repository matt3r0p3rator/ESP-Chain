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
    selectedSSID = "";
    selectedBSSID = "";
    scrollOffset = 0;
    actionMenuIndex = 0;
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
                info.bssid = WiFi.BSSIDstr(i); // Get MAC address
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
    else if (currentState == VIEW_DETAILS) {
        display->clearMenu();
        
        int netIndex = getSelectedNetworkIndex();
        if (netIndex >= 0 && netIndex < networks.size()) {
            NetworkInfo& net = networks[netIndex];
            
            display->drawMenuTitle("Network Details");
            
            // Show details as menu items (non-selectable)
            String ssid = "SSID: " + net.ssid;
            if (ssid.length() > 25) ssid = ssid.substring(0, 25);
            display->drawMenuItem(ssid, 0, false);
            
            String signal = "Signal: " + String(net.rssi) + "dBm";
            display->drawMenuItem(signal, 1, false);
            
            String strength = "Quality: " + getSignalStrength(net.rssi);
            display->drawMenuItem(strength, 2, false);
            
            String security = "Security: " + getEncryptionType(net.encryption);
            display->drawMenuItem(security, 3, false);
            
            String channel = "Channel: " + String(net.channel);
            display->drawMenuItem(channel, 4, false);
            
            display->getTFT()->setTextDatum(TL_DATUM);
            display->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
            display->getTFT()->drawString("BACK: Return", 10, 220, 2);
        }
        
        display->updateMenu();
    }
    else if (currentState == NETWORK_ACTIONS) {
        display->clearMenu();
        
        int netIndex = getSelectedNetworkIndex();
        if (netIndex >= 0 && netIndex < networks.size()) {
            String title = networks[netIndex].ssid;
            if (title.length() > 20) title = title.substring(0, 20);
            display->drawMenuTitle(title.c_str());
            
            display->drawMenuItem("View Details", 0, actionMenuIndex == 0);
            display->drawMenuItem("Save to SD Card", 1, actionMenuIndex == 1);
            display->drawMenuItem("Copy SSID", 2, actionMenuIndex == 2);
            display->drawMenuItem("Back", 3, actionMenuIndex == 3);
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
                // Pause scanning while viewing to prevent list changes
                if (isScanning) {
                    WiFi.scanDelete();
                    isScanning = false;
                }
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
            if (networks.size() == 0) return true; // Safety check
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
            // Validate index after change
            if (selectedIndex >= networks.size()) {
                selectedIndex = max(0, (int)networks.size() - 1);
            }
            drawMenu(displayManager);
        } else if (button == 1) { // Down
            if (networks.size() == 0) return true; // Safety check
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
            // Validate index after change
            if (selectedIndex >= networks.size()) {
                selectedIndex = max(0, (int)networks.size() - 1);
            }
            drawMenu(displayManager);
        } else if (button == 2) { // Select
            // Validate selection is within bounds
            if (selectedIndex >= 0 && selectedIndex < networks.size()) {
                // Double-check bounds and network existence
                if (networks.size() > selectedIndex) {
                    // Store both SSID and BSSID for reliable identification
                    selectedSSID = networks[selectedIndex].ssid;
                    selectedBSSID = networks[selectedIndex].bssid;
                    
                    // Verify we got valid data
                    if (selectedSSID.length() > 0 && selectedBSSID.length() > 0) {
                        // Show network actions menu
                        currentState = NETWORK_ACTIONS;
                        actionMenuIndex = 0;
                        drawMenu(displayManager);
                    }
                }
            }
        } else if (button == 3) { // Back
            currentState = MENU;
            // Validate selectedIndex is still valid
            if (selectedIndex >= networks.size()) {
                selectedIndex = max(0, (int)networks.size() - 1);
            }
            scrollOffset = 0;
            drawMenu(displayManager);
        }
    }
    else if (currentState == NETWORK_ACTIONS) {
        if (button == 0) { // Up
            actionMenuIndex--;
            if (actionMenuIndex < 0) actionMenuIndex = 3;
            drawMenu(displayManager);
        } else if (button == 1) { // Down
            actionMenuIndex++;
            if (actionMenuIndex > 3) actionMenuIndex = 0;
            drawMenu(displayManager);
        } else if (button == 2) { // Select
            if (actionMenuIndex == 0) {
                // View Details - show detailed network info
                currentState = VIEW_DETAILS;
                drawMenu(displayManager);
            } else if (actionMenuIndex == 1) {
                // Save to SD Card
                saveNetworkToSD();
            } else if (actionMenuIndex == 2) {
                // Copy SSID (store in a variable for later use)
                displayManager->getTFT()->fillScreen(TFT_BLACK);
                displayManager->getTFT()->setTextDatum(MC_DATUM);
                displayManager->getTFT()->setTextColor(TFT_GREEN, TFT_BLACK);
                displayManager->getTFT()->drawString("SSID Copied!", 120, 135, 4);
                delay(1000);
                currentState = VIEW_SCAN;
                drawMenu(displayManager);
            } else if (actionMenuIndex == 3) {
                // Back to scan results
                currentState = VIEW_SCAN;
                drawMenu(displayManager);
            }
        } else if (button == 3) { // Back
            currentState = VIEW_SCAN;
            drawMenu(displayManager);
        }
    }
    else if (currentState == VIEW_DETAILS) {
        // Any button returns to actions menu
        currentState = NETWORK_ACTIONS;
        drawMenu(displayManager);
    }

    return true;
}

int WiFiModule::getSelectedNetworkIndex() {
    // First try to match by BSSID (most reliable)
    if (selectedBSSID.length() > 0) {
        for (int i = 0; i < networks.size(); i++) {
            if (networks[i].bssid == selectedBSSID) {
                return i;
            }
        }
    }
    
    // Fallback to SSID match (less reliable due to duplicate SSIDs)
    if (selectedSSID.length() > 0) {
        for (int i = 0; i < networks.size(); i++) {
            if (networks[i].ssid == selectedSSID) {
                return i;
            }
        }
    }
    
    return -1; // Not found
}

void WiFiModule::showNetworkDetails() {
    // Details are now shown in drawMenu
}

void WiFiModule::saveNetworkToSD() {
    int netIndex = getSelectedNetworkIndex();
    if (netIndex < 0 || netIndex >= networks.size()) return;
    
    NetworkInfo& net = networks[netIndex];
    
    displayManager->getTFT()->fillScreen(TFT_BLACK);
    displayManager->getTFT()->setTextDatum(MC_DATUM);
    
    // Check if SD is available
    if (!SD.begin()) {
        displayManager->getTFT()->setTextColor(TFT_RED, TFT_BLACK);
        displayManager->getTFT()->drawString("SD Card Error!", 120, 100, 4);
        displayManager->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
        displayManager->getTFT()->drawString("Insert SD card", 120, 140, 2);
        delay(2000);
        currentState = NETWORK_ACTIONS;
        drawMenu(displayManager);
        return;
    }
    
    // Create or append to wifi_networks.txt
    File file = SD.open("/wifi_networks.txt", FILE_APPEND);
    if (!file) {
        displayManager->getTFT()->setTextColor(TFT_RED, TFT_BLACK);
        displayManager->getTFT()->drawString("File Error!", 120, 120, 4);
        delay(2000);
        currentState = NETWORK_ACTIONS;
        drawMenu(displayManager);
        return;
    }
    
    // Write network info
    file.println("=====================================");
    file.println("SSID: " + net.ssid);
    file.println("RSSI: " + String(net.rssi) + " dBm (" + getSignalStrength(net.rssi) + ")");
    file.println("Encryption: " + getEncryptionType(net.encryption));
    file.println("Channel: " + String(net.channel));
    file.println("Timestamp: " + String(millis() / 1000) + "s");
    file.println("=====================================");
    file.println();
    file.close();
    
    // Show success message
    displayManager->getTFT()->setTextColor(TFT_GREEN, TFT_BLACK);
    displayManager->getTFT()->drawString("Saved!", 120, 100, 4);
    displayManager->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
    displayManager->getTFT()->drawString("wifi_networks.txt", 120, 140, 2);
    delay(1500);
    
    currentState = NETWORK_ACTIONS;
    drawMenu(displayManager);
}
