#pragma once
#include <Arduino.h>
#include "module_base.h"
#include "display_manager.h"
#include "../../ui/icons.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEServer.h>
#include "sd_manager.h"
#include <vector>
#include "ble_spam_utils.h"
#include "ble_jammer_utils.h"

class BLEModule : public Module {
private:
    enum State {
        MENU,
        SCANNER,
        BLESPAM,
        BLEJAMMER
    };
    State currentState;
    int menuIndex;
    int selectedIndex;
    int scrollOffset;
    
    // Scanner members
    BLEScan* pBLEScan;
    bool isScanning;
    std::vector<String> foundDevices;
    std::vector<String> foundAddresses;
    std::vector<int> foundRSSIs;
    std::vector<BLETargetDevice> scannedDevices;
    unsigned long lastScanTime;

    // Spammer members
    bool isSpamming;
    SpamType currentSpamType;
    unsigned long lastSpamUpdate;
    BLEServer* pServer;
    BLEAdvertising* pAdvertising;
    
    // Jammer members
    JammerMode jammerMode;
    
    DisplayManager* displayManager;
    bool initialized;

public:
    BLEModule() : pBLEScan(nullptr), pServer(nullptr), pAdvertising(nullptr), displayManager(nullptr), initialized(false) {}
    void init() override;
    void loop() override;
    String getName() override { return "BLE Tools"; }
    const unsigned char* getIcon() override { return image_bluetooth_bits; }
    int getIconWidth() override { return 16; }
    int getIconHeight() override { return 14; }
    int getIconSpacing() override { return 16 ; }
    int getIconOffsetY() override { return 0; }

    String getDescription() override { return "Bluetooth LE Utilities"; }
    void drawMenu(DisplayManager* display) override;
    bool handleInput(uint8_t button) override;
    
    // Helper methods
    void startScan();
    void stopScan();
    void toggleSpam();
    void selectDeviceForJammer(int index);
    void toggleJammer();
};