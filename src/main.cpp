#include <Arduino.h>
#include "display_manager.h"
#include "menu_system.h"
#include "input_manager.h"
#include "module_base.h"
#include "driver/rtc_io.h"
#include "modules/wifi/wifi_module.h"
#include "modules/counter_module.h" // 1. Include your new module header
#include "modules/file_explorer_module.h"
#include "modules/firmware_upgrade.h"
#include "modules/settings_module.h"
#include "modules/gps/gps_module.h"
#include "modules/compass/compass_module.h"
#include "badusb_module.h"
#include "sd_manager.h"
#include "config_manager.h"

// --- Sleep Module ---
class SleepModule : public Module {
public:
    void init() override {
        // Configure Wakeup on Button 14 (Low)
        // Ensure pullup is enabled for the button during sleep
        rtc_gpio_pullup_en(GPIO_NUM_14);
        rtc_gpio_pulldown_dis(GPIO_NUM_14);
        esp_sleep_enable_ext1_wakeup(1ULL << 14, ESP_EXT1_WAKEUP_ANY_LOW);
    }

    void loop() override {
        delay(1000); // Give time to see the message
        
        // Turn off display
        // We need access to display manager, but it's passed in drawMenu. 
        // We can access the global one or cast from the one passed in drawMenu if we stored it.
        // Since we are in main.cpp, we can access the global 'displayManager' object if we declare it extern or just use it since it's in the same file.
        extern DisplayManager displayManager;
        displayManager.turnOff();
        
        esp_deep_sleep_start();
    }

    String getName() override {
        return "Deep Sleep";
    }

    String getDescription() override {
        return "Enter Deep Sleep";
    }

    void drawMenu(DisplayManager* display) override {
        display->clearContent();
        display->drawMenuTitle("Deep Sleep");
        
        display->getTFT()->setTextDatum(MC_DATUM);
        display->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK); 
        display->getTFT()->drawString("Going to sleep...", 160, 80, 4);
        display->getTFT()->drawString("Press Btn 14", 160, 120, 2);
        display->getTFT()->drawString("to wake up", 160, 140, 2);
    }

    bool handleInput(uint8_t button) override {
        return true;
    }
};

// --- Simple About Module ---
class AboutModule : public Module {
public:
    void init() override {
        // Nothing to init
    }
    
    void loop() override {
        // Nothing to loop
    }
    
    String getName() override {
        return "About";
    }
    
    String getDescription() override {
        return "System Info";
    }
    
    void drawMenu(DisplayManager* display) override {
        display->clearContent();
        
        display->getTFT()->setTextDatum(ML_DATUM);
        display->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
        
        display->getTFT()->drawString("ESP-Chain Matt3r FW beta v0", 20, 60, 2);
        display->getTFT()->drawString("LilyGo T-Display S3", 20, 85, 2);
        display->getTFT()->drawString("Chip: ESP32-S3", 20, 110, 2);
        
        String flashSize = "Flash: " + String(ESP.getFlashChipSize() / (1024 * 1024)) + "MB";
        display->getTFT()->drawString(flashSize, 20, 135, 2);
        
        display->getTFT()->drawString("Long Press Btn 14", 20, 180, 2);
        display->getTFT()->drawString("to exit", 20, 205, 2);
    }
    
    bool handleInput(uint8_t button) override {
        if (button == 3) return false; // Exit
        return true;
    }
};

// --- Global Objects ---
DisplayManager displayManager;
SDManager sdManager;
MenuSystem menuSystem(&displayManager, &sdManager);
InputManager inputManager(&menuSystem);

// 2. Instantiate your modules
AboutModule aboutModule;
SleepModule sleepModule;
WiFiModule wifiModule;
CounterModule counterModule;
FileExplorerModule fileExplorerModule;
FirmwareUpgradeModule firmwareUpgradeModule;
SettingsModule settingsModule;
GPSModule gpsModule;
CompassModule compassModule;
BadUSBModule badusbModule;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting ESP-Chain...");

    // Initialize Display
    displayManager.init();
    displayManager.drawStatusBar("Booting...", displayManager.getBatteryVoltage(), false);

    // Initialize SD Card
    if (sdManager.init()) {
        Serial.println("SD Card Initialized");
        if (ConfigManager::getInstance().load()) {
             Serial.println("Config Loaded");
        } else {
             Serial.println("Config Load Failed or Missing. Creating defaults...");
             ConfigManager::getInstance().save();
        }
    } else {
        Serial.println("SD Card Failed");
        displayManager.drawStatusBar("SD Failed", displayManager.getBatteryVoltage(), false);
    }

    delay(1000);

    // Initialize Input
    inputManager.begin();

    // 3. Register Modules
    // The order here determines the order in the menu!
    // To move items, just cut and paste these lines.
    
    //menuSystem.registerModule(&counterModule);
    menuSystem.registerModule(&wifiModule);
    menuSystem.registerModule(&gpsModule);
    menuSystem.registerModule(&compassModule);
    menuSystem.registerModule(&badusbModule);
    menuSystem.registerModule(&fileExplorerModule);
    menuSystem.registerModule(&sleepModule);
    menuSystem.registerModule(&settingsModule);
    //menuSystem.registerModule(&firmwareUpgradeModule);
    menuSystem.registerModule(&aboutModule);

    // Initial Draw
    menuSystem.draw();
}

void loop() {
    inputManager.update();
    menuSystem.update();
    delay(10); // Small delay to prevent watchdog issues if loop is too tight
}
