#include "menu_system.h"
#include "driver/rtc_io.h"

#define PIN_EXT_POWER 17

MenuSystem::MenuSystem(DisplayManager* display, SDManager* sd) : displayManager(display), sdManager(sd), selectedIndex(0), scrollOffset(0), inModule(false), activeModule(nullptr), isDeepSleepPending(false), deepSleepStartTime(0) {}

void MenuSystem::registerModule(Module* module) {
    modules.push_back(module);
}

void MenuSystem::draw() {
    // Always draw status bar
    String statusText = inModule && activeModule ? activeModule->getName() : "Main Menu";
    displayManager->drawStatusBar(statusText, displayManager->getBatteryVoltage(), sdManager->isMounted());

    if (inModule && activeModule) {
        activeModule->drawMenu(displayManager);
    } else {
        displayManager->clearContent();
        // displayManager->drawMenuTitle("ESP-Chain"); // Removed title
        
        int start = scrollOffset;
        int end = start + itemsPerPage;
        if (end > modules.size()) end = modules.size();

        for (int i = start; i < end; i++) {
            displayManager->drawMenuItem(modules[i]->getName(), i - scrollOffset, i == selectedIndex);
        }
    }
}

void MenuSystem::handleInput(uint8_t input) {
    if (isDeepSleepPending) {
        isDeepSleepPending = false;
        draw();
        return;
    }

    if (inModule && activeModule) {
        // Pass input to module. If it returns false, it means it wants to exit.
        if (!activeModule->handleInput(input)) {
            inModule = false;
            activeModule = nullptr;
            draw();
        }
        return;
    }

    // Main Menu Navigation
    switch (input) {
        case 0: // Up (Not used)
            break;
        case 1: // Down (Single Click)
            if (selectedIndex < modules.size() - 1) {
                selectedIndex++;
                if (selectedIndex >= scrollOffset + itemsPerPage) {
                    scrollOffset++;
                }
            } else {
                selectedIndex = 0;
                scrollOffset = 0;
            }
            draw();
            break;
        case 2: // Select (Double Click)
            if (modules.size() > 0) {
                activeModule = modules[selectedIndex];
                inModule = true;
                displayManager->clearContent(); // Clear only content area
                activeModule->init();
                // activeModule->drawMenu(displayManager); // Let the loop handle drawing or call it here
                draw();
            }
            break;
        case 3: // Back (Long Press)
            if (!inModule) {
                isDeepSleepPending = true;
                deepSleepStartTime = millis();
                displayManager->clearContent();
                displayManager->getTFT()->setTextDatum(MC_DATUM);
                displayManager->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
                displayManager->getTFT()->drawString("Sleeping in 5s...", 160, 80, 4);
                displayManager->getTFT()->drawString("Press any key", 160, 120, 2);
                displayManager->getTFT()->drawString("to cancel", 160, 140, 2);
            }
            break;
    }
}

void MenuSystem::update() {
    // Update clock every second
    static unsigned long lastClockUpdate = 0;
    if (millis() - lastClockUpdate > 1000) { 
        lastClockUpdate = millis();
        if (!isDeepSleepPending) {
            displayManager->updateClock();
        }
    }
    
    // Update full status bar (battery etc) every 30 seconds
    static unsigned long lastStatusUpdate = 0;
    if (millis() - lastStatusUpdate > 30000) {
        lastStatusUpdate = millis();
        if (!isDeepSleepPending) {
            String statusText = inModule && activeModule ? activeModule->getName() : "Main Menu";
            displayManager->drawStatusBar(statusText, displayManager->getBatteryVoltage(), sdManager->isMounted());
        }
    }

    if (isDeepSleepPending) {
        unsigned long elapsed = millis() - deepSleepStartTime;
        int remaining = 5 - (elapsed / 1000);
        
        if (remaining <= 0) {
            enterDeepSleep();
        } else {
            static int lastRemaining = -1;
            if (remaining != lastRemaining) {
                lastRemaining = remaining;
                displayManager->getTFT()->setTextDatum(MC_DATUM);
                displayManager->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
                String msg = "Sleeping in " + String(remaining) + "s...";
                displayManager->getTFT()->fillRect(0, 60, 320, 40, TFT_BLACK);
                displayManager->getTFT()->drawString(msg, 160, 80, 4);
            }
        }
        return;
    }

    if (inModule && activeModule) {
        activeModule->loop();
    }
}

void MenuSystem::enterDeepSleep() {
    // Turn off display
    displayManager->turnOff();

    // Turn off GPS/I2C Power
    pinMode(PIN_EXT_POWER, OUTPUT);
    digitalWrite(PIN_EXT_POWER, LOW); // Turn OFF NPN
    gpio_hold_en((gpio_num_t)PIN_EXT_POWER); // Hold the state in deep sleep
    
    // Configure Wakeup on Button 14 (Low)
    rtc_gpio_pullup_en(GPIO_NUM_14);
    rtc_gpio_pulldown_dis(GPIO_NUM_14);
    esp_sleep_enable_ext1_wakeup(1ULL << 14, ESP_EXT1_WAKEUP_ANY_LOW);

    esp_deep_sleep_start();
}
