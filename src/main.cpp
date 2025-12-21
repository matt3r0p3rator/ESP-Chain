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
#include "modules/i2c_scanner_module.h"
#include "badusb_module.h"
#include "sd_manager.h"
#include "config_manager.h"

// --- Sleep Module ---
class SleepModule : public Module {
public:
    int PIN_EXT_POWER = 17;
    void init() override {
        // Configure Wakeup on Button 14 (Low)
        // Ensure pullup is enabled for the button during sleep
        // Formally stop SD card operations
        extern SDManager sdManager;
        sdManager.end();

        // Prevent phantom power to SD card by grounding SPI pins
        pinMode(10, OUTPUT); digitalWrite(10, LOW); // CS
        pinMode(11, OUTPUT); digitalWrite(11, LOW); // MOSI
        pinMode(12, OUTPUT); digitalWrite(12, LOW); // SCK
        pinMode(13, INPUT_PULLDOWN);                // MISO

        delay(400); // Allow time for any previous operations
        pinMode(PIN_EXT_POWER, OUTPUT);
        digitalWrite(PIN_EXT_POWER, LOW); // Turn OFF NPN
        gpio_hold_en((gpio_num_t)PIN_EXT_POWER); // Hold the state in deep sleep
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
BadUSBModule badusbModule;
I2CScannerModule i2cScannerModule;

int PIN_EXT_POWER = 17;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting ESP-Chain...");

    // Initialize Display
    displayManager.init();
    displayManager.initRTC();
    displayManager.getTFT()->setTextDatum(MC_DATUM);
    displayManager.getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
    displayManager.drawStatusBar("Booting...", displayManager.getBatteryVoltage(), false, false, "ESP-Chain");
    pinMode(PIN_EXT_POWER, OUTPUT);
    gpio_hold_dis((gpio_num_t)PIN_EXT_POWER); // Disable hold before writing
    
    // Cycle power to ensure SD card reset
    digitalWrite(PIN_EXT_POWER, LOW);
    delay(250);
    digitalWrite(PIN_EXT_POWER, HIGH); // Turn ON NPN

    displayManager.getTFT()->drawString("Wait for External Power", 160, 40, 2);
    delay(700); // Wait for SPI devices to power up
    //Write initial status to tft
    displayManager.clearContent();
    displayManager.drawStatusBar("Initializing...", displayManager.getBatteryVoltage(), false, true);
    displayManager.getTFT()->drawString("Initializing SD Card...", 160, 40, 2);
    

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
        displayManager.getTFT()->drawString("SD Card Failed", 160, 80, 2);
        delay(2000);
    }

    delay(1000);

    // Initialize Input
    inputManager.begin();

    // 3. Register Modules
    // The order here determines the order in the menu!
    // To move items, just cut and paste these lines.
    
    menuSystem.registerModule(&wifiModule);
    menuSystem.registerModule(&badusbModule);
    menuSystem.registerModule(&fileExplorerModule);
    menuSystem.registerModule(&sleepModule);
    menuSystem.registerModule(&settingsModule);
    menuSystem.registerModule(&i2cScannerModule);
    menuSystem.registerModule(&aboutModule);

    // Initial Draw
    menuSystem.draw();
}

void loop() {
    inputManager.update();
    menuSystem.update();
    delay(10); // Small delay to prevent watchdog issues if loop is too tight
}
