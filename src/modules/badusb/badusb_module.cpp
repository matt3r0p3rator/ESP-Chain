#include "badusb_module.h"
#include "ducky_parser.h"
#include "display_manager.h"
#include "sd_manager.h"
#include "USBHIDKeyboard.h"
#include "USB.h"
#include "config_manager.h"
#include "../../ui/icons.h"

extern SDManager sdManager;
extern DisplayManager displayManager;

USBHIDKeyboard Keyboard;
DuckyParser parser(&Keyboard);

void BadUSBModule::init() {
    Keyboard.begin();
    USB.begin();
    state = STATE_BROWSING;
    statusMessage = "";
}

void BadUSBModule::loop() {
    if (state == STATE_ARMED) {
        // USB Detection is not reliable on all cores/settings without TinyUSB
        // We will proceed to delay immediately.
        state = STATE_WAITING_DELAY;
        armedTime = millis();
        drawMenu(&displayManager);
    } else if (state == STATE_WAITING_DELAY) {
        // Update countdown on screen every 100ms
        static unsigned long lastUpdate = 0;
        if (millis() - lastUpdate > 100) {
            lastUpdate = millis();
            drawMenu(&displayManager);
        }

        if (millis() - armedTime > (unsigned long)ConfigManager::getInstance().data.badusbStartupDelay) {
             state = STATE_RUNNING;
             drawMenu(&displayManager);
             
             parser.parseFile(selectedPayload);
             
             state = STATE_DONE;
             drawMenu(&displayManager);
        }
    }
}

String BadUSBModule::getName() {
    return "BadUSB";
}

const unsigned char* BadUSBModule::getIcon() {
    return image_badusb_bits;
}

int BadUSBModule::getIconWidth() {
    return 24;
}

int BadUSBModule::getIconHeight() {
    return 16;
}

String BadUSBModule::getDescription() {
    return "Run Ducky Scripts";
}

void BadUSBModule::drawMenu(DisplayManager* display) {
    display->clearContent();
    
    if (state == STATE_ARMED) {
        display->drawMenuTitle("ARMED");
        display->getTFT()->setTextDatum(MC_DATUM);
        display->getTFT()->setTextColor(TFT_RED, TFT_BLACK);
        display->getTFT()->drawString("WAITING USB...", 160, 80, 4);
        display->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
        display->getTFT()->drawString("Plug in USB now", 160, 120, 2);
        display->getTFT()->drawString("Long Press to Cancel", 160, 200, 2);
        return;
    } else if (state == STATE_WAITING_DELAY) {
        display->drawMenuTitle("ARMED");
        display->getTFT()->setTextDatum(MC_DATUM);
        display->getTFT()->setTextColor(TFT_ORANGE, TFT_BLACK);
        display->getTFT()->drawString("STARTING...", 160, 80, 4);
        display->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
        
        long remaining = (long)ConfigManager::getInstance().data.badusbStartupDelay - (long)(millis() - armedTime);
        if (remaining < 0) remaining = 0;
        display->getTFT()->drawString("Running in " + String(remaining) + "ms", 160, 120, 2);
        
        display->getTFT()->drawString("Double Click to Skip", 160, 160, 2);
        display->getTFT()->drawString("Long Press to Cancel", 160, 200, 2);
        return;
    } else if (state == STATE_RUNNING) {
        display->drawMenuTitle("RUNNING");
        display->getTFT()->setTextDatum(MC_DATUM);
        display->getTFT()->setTextColor(TFT_YELLOW, TFT_BLACK);
        display->getTFT()->drawString("EXECUTING...", 160, 100, 4);
        return;
    } else if (state == STATE_DONE) {
        display->drawMenuTitle("DONE");
        display->getTFT()->setTextDatum(MC_DATUM);
        display->getTFT()->setTextColor(TFT_GREEN, TFT_BLACK);
        display->getTFT()->drawString("FINISHED", 160, 100, 4);
        display->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
        display->getTFT()->drawString("Press Back", 160, 140, 2);
        return;
    }

    String title = "BadUSB";
    if (currentPath != "/payloads") {
        title = currentPath.substring(currentPath.lastIndexOf('/') + 1);
    }
    display->drawMenuTitle(title);
    
    if (!filesLoaded) {
        scriptFiles = sdManager.listDir(currentPath);
        filesLoaded = true;
    }
    
    if (scriptFiles.empty()) {
        display->getTFT()->drawString("Empty folder", 20, 60, 2);
        return;
    }

    int maxItems = 5; 
    int startIdx = 0;
    if (selectedIndex >= maxItems) {
        startIdx = selectedIndex - maxItems + 1;
    }
    
    // Ensure startIdx is valid
    if (startIdx < 0) startIdx = 0;
    if (startIdx > (int)scriptFiles.size() - maxItems && (int)scriptFiles.size() > maxItems) {
        startIdx = scriptFiles.size() - maxItems;
    }

    for (int i = startIdx; i < (int)scriptFiles.size() && i < startIdx + maxItems; i++) {
        String name = scriptFiles[i].name;
        if (name.startsWith("/")) name = name.substring(name.lastIndexOf('/') + 1);
        if (scriptFiles[i].isDirectory) name += "/";
        
        display->drawMenuItem(name, i - startIdx, i == selectedIndex);
    }
    
    if (statusMessage != "") {
        display->getTFT()->setTextColor(TFT_GREEN, TFT_BLACK);
        display->getTFT()->drawString(statusMessage, 20, 180, 2);
    }
}

bool BadUSBModule::handleInput(uint8_t button) {
    if (state == STATE_ARMED || state == STATE_RUNNING || state == STATE_WAITING_DELAY) {
        if (button == 3) { // Cancel
            state = STATE_BROWSING;
            drawMenu(&displayManager);
        } else if (state == STATE_WAITING_DELAY && button == 2) { // Select to Skip
             state = STATE_RUNNING;
             drawMenu(&displayManager);
             
             parser.parseFile(selectedPayload);
             
             state = STATE_DONE;
             drawMenu(&displayManager);
        }
        return true;
    } else if (state == STATE_DONE) {
        if (button == 3) {
            state = STATE_BROWSING;
            drawMenu(&displayManager);
        }
        return true;
    }

    if (button == 1) { // Down / Next
        if (!scriptFiles.empty()) {
            selectedIndex++;
            if (selectedIndex >= (int)scriptFiles.size()) {
                selectedIndex = 0;
            }
            drawMenu(&displayManager);
        }
    } else if (button == 2) { // Select
        if (selectedIndex >= 0 && selectedIndex < (int)scriptFiles.size()) {
            FileEntry& entry = scriptFiles[selectedIndex];
            String fullPath = entry.name;
            if (!fullPath.startsWith("/")) fullPath = currentPath + "/" + fullPath;
            
            if (entry.isDirectory) {
                currentPath = fullPath;
                filesLoaded = false;
                selectedIndex = 0;
                statusMessage = "";
                drawMenu(&displayManager);
            } else {
                selectedPayload = fullPath;
                state = STATE_ARMED;
                armedTime = millis();
                drawMenu(&displayManager);
            }
        }
    } else if (button == 3) { // Back
        if (currentPath == "/payloads") {
            return false;
        } else {
            int lastSlash = currentPath.lastIndexOf('/');
            if (lastSlash > 0) { // e.g. /payloads/windows -> /payloads
                currentPath = currentPath.substring(0, lastSlash);
            } else {
                currentPath = "/payloads";
            }
            filesLoaded = false;
            selectedIndex = 0;
            statusMessage = "";
            drawMenu(&displayManager);
        }
    }
    return true;
}
