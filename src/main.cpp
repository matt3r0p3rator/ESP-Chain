#include <Arduino.h>
#include "display_manager.h"
#include "sd_manager.h"
#include "config_manager.h"
#include "menu_system.h"
#include "badusb_module.h"
#include "input_manager.h"

DisplayManager* displayManager;
SDManager* sdManager;
ConfigManager* configManager;
MenuSystem* menuSystem;
InputManager* inputManager;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting Setup...");
    
    // Init Core
    Serial.println("Init Managers...");
    displayManager = new DisplayManager();
    sdManager = new SDManager();
    configManager = new ConfigManager(sdManager);
    menuSystem = new MenuSystem(displayManager);
    inputManager = new InputManager(menuSystem);

    Serial.println("Init Display...");
    displayManager->init();
    
    Serial.println("Init SD...");
    // Initialize SD with specific CS pin (15)
    if (!SD.begin(15)) {
        Serial.println("SD Init Failed");
        displayManager->drawStatusBar("SD Fail", 0);
    } else {
        Serial.println("SD Init Success");
        Serial.println("Loading Config...");
        configManager->loadConfig();
    }
    
    // Register Modules
    Serial.println("Registering Modules...");
    menuSystem->registerModule(new BadUSBModule());
    
    // Init Input
    Serial.println("Init Input...");
    inputManager->begin();
    
    // Initial Draw
    Serial.println("Drawing Menu...");
    menuSystem->draw();
    Serial.println("Setup Complete");
}

void loop() {
    inputManager->update();
    menuSystem->update();
    delay(10);
}
