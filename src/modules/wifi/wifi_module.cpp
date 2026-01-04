#include "wifi_module.h"
#include "display_manager.h"

// Menu Items
const char* menuItems[] = {
    "Scan Networks",
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
    display->clearContent();

    if (currentState == MENU) {
        display->drawMenuTitle("WiFi Tools");
        for (int i = 0; i < menuItemsCount; i++) {
            display->drawMenuItem(menuItems[i], i, selectedIndex == i);
        }
    } else {
        // Draw sub-menu or tool interface based on currentState
        switch (currentState) {
            case SCAN_NETWORKS:
                display->drawMenuTitle("Scan Networks");
                display->getTFT()->setTextColor(TFT_WHITE);
                display->getTFT()->setTextDatum(TL_DATUM);
                display->getTFT()->drawString("Functionality TBD", 10, 40, 2);
                break;
            case EVIL_PORTAL:
                display->drawMenuTitle("Evil Portal");
                display->getTFT()->setTextColor(TFT_WHITE);
                display->getTFT()->setTextDatum(TL_DATUM);
                display->getTFT()->drawString("Functionality TBD", 10, 40, 2);
                break;
            case KARMA_ATTACK:
                display->drawMenuTitle("Karma Attack");
                display->getTFT()->setTextColor(TFT_WHITE);
                display->getTFT()->setTextDatum(TL_DATUM);
                display->getTFT()->drawString("Functionality TBD", 10, 40, 2);
                break;
            case RESPONDER:
                display->drawMenuTitle("Responder");
                display->getTFT()->setTextColor(TFT_WHITE);
                display->getTFT()->setTextDatum(TL_DATUM);
                display->getTFT()->drawString("Functionality TBD", 10, 40, 2);
                break;
            case SNIFFER:
                display->drawMenuTitle("Sniffer");
                display->getTFT()->setTextColor(TFT_WHITE);
                display->getTFT()->setTextDatum(TL_DATUM);
                display->getTFT()->drawString("Functionality TBD", 10, 40, 2);
                break;
            case DEAUTH_ATTACK:
                display->drawMenuTitle("Deauth Attack");
                display->getTFT()->setTextColor(TFT_WHITE);
                display->getTFT()->setTextDatum(TL_DATUM);
                display->getTFT()->drawString("Functionality TBD", 10, 40, 2);
                break;
            case CAPTURE_HANDSHAKE:
                display->drawMenuTitle("Capture Handshake");
                display->getTFT()->setTextColor(TFT_WHITE);
                display->getTFT()->setTextDatum(TL_DATUM);
                display->getTFT()->drawString("Functionality TBD", 10, 40, 2);
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
                selectedIndex = menuItemsCount - 1; // Wrap around
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
            if (selectedIndex >= menuItemsCount) {
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
            currentState = static_cast<State>(selectedIndex + 1); // +1 to skip MENU state
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
