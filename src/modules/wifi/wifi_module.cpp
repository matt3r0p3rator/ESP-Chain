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
    currentState = MENU;
    menuIndex = 0;
    selectedIndex = 0;
    scrollOffset = 0;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
}

void WiFiModule::loop() {
    // Main loop logic for active tools would go here
    // For example, if (currentState == EVIL_PORTAL) evilPortal.loop();
}

void WiFiModule::drawMenu(DisplayManager* display) {
    this->displayManager = display; // Store for later use if needed
    // display->clearContent();

    if (currentState == MENU) {
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
                display->drawMenuTitle("Scan Results");
                
                if (scanResults.empty()) {
                    display->getTFT()->setTextColor(TFT_WHITE);
                    display->getTFT()->setTextDatum(MC_DATUM);
                    display->getTFT()->drawString(isScanning ? "Scanning..." : "NOT SCANNING", 10, 40, 2);
                } else {
                    display->clearMenu();
                    // Draw list
                    int itemsPerPage = 5; // Assuming 5 fits
                    int count = scanResults.size();
                    
                    for (int i = 0; i < itemsPerPage; i++) {
                        int idx = scrollOffset + i;
                        if (idx >= count) break;
                        
                        String label = scanResults[idx];
                        display->drawMenuItem(label, i, selectedIndex == idx);
                    }
                    display->drawScrollBar(count, scrollOffset, itemsPerPage);
                    display->updateMenu();
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
            if (selectedIndex == 0) { // Scan Networks
                if (isScanning) {
                    // Stop scan
                    isScanning = false;
                    WiFi.scanDelete();
                } else {
                    // Start scan
                    isScanning = true;
                    scanResults.clear();
                    networkCount = WiFi.scanNetworks(true);
                }
                drawMenu(displayManager); // Force redraw
                return true;
            }
            currentState = static_cast<State>(selectedIndex); // Map directly to State
            drawMenu(displayManager); // Force redraw
        } else if (button == 3) { // Back
            return false; // Exit module
        }
    } else {
        if (button == 3) { // Back
            currentState = MENU;
            drawMenu(displayManager); // Force redraw
        }
    }
    return true; // Continue in module
}
