#pragma once
#include "module_base.h"
#include "display_manager.h"
#include "sd_manager.h"
#include "USB.h"
#include "USBMSC.h"
#include "SdFat.h"

// Pins matching sd_manager.cpp
#define SD_CS_PIN   10
#define SD_MOSI_PIN 11
#define SD_SCK_PIN  12
#define SD_MISO_PIN 13

extern SDManager sdManager;
extern DisplayManager displayManager;

// Global objects for MSC callbacks
static SdFat sdFat;
static USBMSC MSC;

// Callbacks
static int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
    if (!sdFat.card()->readSectors(lba, (uint8_t*)buffer, bufsize / 512)) return -1;
    return bufsize;
}

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
    if (!sdFat.card()->writeSectors(lba, buffer, bufsize / 512)) return -1;
    return bufsize;
}

class USBStorageModule : public Module {
    bool isRunning = false;
    bool initFailed = false;

public:
    void init() override {
        // Initialization is done when entering the module
    }

    void loop() override {
        if (!isRunning && !initFailed) {
            startMSC();
        }
    }

    String getName() override {
        return "USB Storage";
    }

    String getDescription() override {
        return "Mass Storage Mode";
    }

    void drawMenu(DisplayManager* display) override {
        display->clearContent();
        display->drawMenuTitle("USB Storage");
        
        display->getTFT()->setTextDatum(MC_DATUM);
        display->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK); 
        
        if (initFailed) {
            display->getTFT()->drawString("SD Init Failed!", 160, 80, 4);
            display->getTFT()->drawString("Press Btn to Exit", 160, 120, 2);
        } else if (isRunning) {
            display->getTFT()->drawString("USB Active", 160, 80, 4);
            display->getTFT()->drawString("Connect to PC", 160, 120, 2);
            display->getTFT()->drawString("Press RST to Exit", 160, 160, 2);
        } else {
            display->getTFT()->drawString("Starting...", 160, 80, 4);
        }
    }

    bool handleInput(uint8_t button) override {
        // Any button press exits
        stopMSC();
        return false; 
    }
    
    void startMSC() {
        if (isRunning) return;
        
        // 1. Stop system SD
        sdManager.end();
        
        // 2. Init SdFat
        // Use the same SPI pins. We use the global SPI object which is already configured.
        if (!sdFat.begin(SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(16), &SPI))) {
             initFailed = true;
             drawMenu(&displayManager);
             return;
        }
        
        // 3. Init MSC
        MSC.vendorID("ESP32");
        MSC.productID("USB_MSC");
        MSC.productRevision("1.0");
        MSC.onRead(onRead);
        MSC.onWrite(onWrite);
        MSC.mediaPresent(true);
        
        uint32_t secCount = sdFat.card()->sectorCount();
        MSC.begin(secCount, 512);
        USB.begin();
        
        isRunning = true;
        drawMenu(&displayManager);
    }
    
    void stopMSC() {
        if (!isRunning) {
            // Even if not running, ensure SD is back
            if (!sdManager.isMounted()) sdManager.init();
            return;
        }
        
        MSC.mediaPresent(false);
        MSC.end();
        
        delay(500);
        
        // Re-init system SD
        sdManager.init();
        
        isRunning = false;
        initFailed = false;
    }
};
