#pragma once
#include "module_base.h"
#include "display_manager.h"
#include "sd_manager.h"
#include "config_manager.h"
#include "../ui/icons.h"
#include "../ui/themes.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEService.h>
#include <BLECharacteristic.h>
#include <BLE2902.h>
#include <SD.h>

extern SDManager sdManager;
extern DisplayManager displayManager;

// BLE Service and Characteristic UUIDs for file transfer
#define FILE_TRANSFER_SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define FILE_LIST_CHARACTERISTIC_UUID     "12345678-1234-1234-1234-123456789abd" 
#define FILE_READ_CHARACTERISTIC_UUID     "12345678-1234-1234-1234-123456789abe"
#define FILE_WRITE_CHARACTERISTIC_UUID    "12345678-1234-1234-1234-123456789abf"
#define FILE_COMMAND_CHARACTERISTIC_UUID  "12345678-1234-1234-1234-123456789ac0"

// Forward declarations
class BLEStorageServerCallbacks;
class FileCommandCallbacks;
class FileWriteCallbacks;

class BLEStorageModule : public Module {
    friend class BLEStorageServerCallbacks;
    friend class FileCommandCallbacks;
    friend class FileWriteCallbacks;
private:
    enum State {
        STOPPED,
        STARTING,
        RUNNING,
        ERROR
    };
    
    State currentState;
    String statusMessage;
    bool isRunning;
    String deviceName;
    
    // BLE components
    BLEServer* pServer;
    BLEService* pService;
    BLECharacteristic* pFileListChar;
    BLECharacteristic* pFileReadChar;
    BLECharacteristic* pFileWriteChar;
    BLECharacteristic* pFileCommandChar;
    
    // File transfer state
    File currentFile;
    String currentFilePath;
    bool fileTransferActive;
    uint32_t transferredBytes;
    uint32_t totalBytes;
    
    // Client connection
    bool deviceConnected;
    bool oldDeviceConnected;
    
    // UI state
    unsigned long lastUIUpdate;
    int connectedClients;

public:
    BLEStorageModule() : 
        currentState(STOPPED),
        isRunning(false),
        deviceName("ESP-Chain-Storage"),
        pServer(nullptr),
        pService(nullptr),
        pFileListChar(nullptr),
        pFileReadChar(nullptr),
        pFileWriteChar(nullptr),
        pFileCommandChar(nullptr),
        fileTransferActive(false),
        transferredBytes(0),
        totalBytes(0),
        deviceConnected(false),
        oldDeviceConnected(false),
        lastUIUpdate(0),
        connectedClients(0)
    {}

    void init() override {}

    void loop() override {
        handleConnectionState();
        updateUI();
    }

    String getName() override {
        return "BLE Storage";
    }

    const unsigned char* getIcon() override {
        return image_bluetooth_bits;
    }

    int getIconWidth() override { return 16; }
    int getIconHeight() override { return 14; }
    int getIconSpacing() override { return 18; }
    int getIconOffsetY() override { return 0; }

    String getDescription() override {
        return "Bluetooth LE File Transfer";
    }

    void drawMenu(DisplayManager* display) override {
        display->clearContent();
        display->drawMenuTitle("BLE Storage");
        
        display->getTFT()->setTextDatum(TL_DATUM);
        display->getTFT()->setTextColor(THEME_TEXT, THEME_BG); 
        
        // Status
        String stateText = "";
        switch(currentState) {
            case STOPPED:
                stateText = "Status: Stopped";
                break;
            case STARTING:
                stateText = "Status: Starting...";
                break;
            case RUNNING:
                stateText = "Status: Running";
                break;
            case ERROR:
                stateText = "Status: Error";
                break;
        }
        display->getTFT()->drawString(stateText, 20, 30, 2);
        
        if (currentState == RUNNING) {
            // Device name and connection info
            display->getTFT()->drawString("Device: " + deviceName, 20, 50, 2);
            display->getTFT()->drawString("Connected: " + String(deviceConnected ? "Yes" : "No"), 20, 70, 2);
            
            if (fileTransferActive) {
                // Transfer progress
                display->getTFT()->drawString("Transferring:", 20, 90, 2);
                String fileName = currentFilePath.substring(currentFilePath.lastIndexOf('/') + 1);
                display->getTFT()->drawString(fileName, 20, 110, 2);
                
                if (totalBytes > 0) {
                    int progress = (transferredBytes * 100) / totalBytes;
                    String progressText = String(transferredBytes) + "/" + String(totalBytes) + " (" + String(progress) + "%)";
                    display->getTFT()->drawString(progressText, 20, 130, 2);
                    
                    // Progress bar
                    int barWidth = 120;
                    int barHeight = 8;
                    int barX = 20;
                    int barY = 150;
                    
                    display->getTFT()->drawRect(barX, barY, barWidth, barHeight, THEME_TEXT);
                    if (progress > 0) {
                        int fillWidth = (barWidth * progress) / 100;
                        display->getTFT()->fillRect(barX + 1, barY + 1, fillWidth, barHeight - 2, THEME_ACCENT);
                    }
                }
            }
        } else if (currentState == ERROR) {
            display->getTFT()->setTextColor(TFT_RED, THEME_BG);
            display->getTFT()->drawString("Error: " + statusMessage, 20, 50, 2);
            display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
        }
        
        // Instructions
        if (currentState == STOPPED) {
            display->getTFT()->drawString("Btn 0: Start", 20, 190, 2);
            display->getTFT()->drawString("Btn 1: Back", 20, 210, 2);
        } else if (currentState == RUNNING) {
            display->getTFT()->drawString("Btn 0: Stop", 20, 190, 2);
            display->getTFT()->drawString("Btn 1: Back", 20, 210, 2);
        }
    }

    bool handleInput(uint8_t button) override {
        switch(button) {
            case 0: // OK button
                if (currentState == STOPPED || currentState == ERROR) {
                    startBLEServer();
                } else if (currentState == RUNNING) {
                    stopBLEServer();
                }
                return true;
                
            case 1: // BACK button
                if (currentState == RUNNING) {
                    stopBLEServer();
                }
                return false;
                
            default:
                return true;
        }
    }

    bool isBackgroundRunning() override {
        return (currentState == RUNNING);
    }

    void backgroundLoop() override {
        if (currentState == RUNNING) {
            handleConnectionState();
        }
    }

private:
    void startBLEServer();
    void stopBLEServer();
    void setupBLEService();
    void handleConnectionState();
    void updateUI();
    String listFiles(String path);
    void handleFileCommand(String command);
    void startFileRead(String filePath);
    void startFileWrite(String filePath);
    void continueFileTransfer();
};

// BLE Server callbacks
class BLEStorageServerCallbacks: public BLEServerCallbacks {
private:
    BLEStorageModule* module;
    
public:
    BLEStorageServerCallbacks(BLEStorageModule* mod) : module(mod) {}
    
    void onConnect(BLEServer* pServer) {
        // Handle connection in the module
    };

    void onDisconnect(BLEServer* pServer) {
        // Handle disconnection in the module
    }
};

// BLE Characteristic callbacks
class FileCommandCallbacks: public BLECharacteristicCallbacks {
private:
    BLEStorageModule* module;
    
public:
    FileCommandCallbacks(BLEStorageModule* mod) : module(mod) {}
    
    void onWrite(BLECharacteristic* pCharacteristic) {
        String command = String(pCharacteristic->getValue().c_str());
        module->handleFileCommand(command);
    }
};

// File write callbacks
class FileWriteCallbacks: public BLECharacteristicCallbacks {
private:
    BLEStorageModule* module;
    
public:
    FileWriteCallbacks(BLEStorageModule* mod) : module(mod) {}
    
    void onWrite(BLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            // Write data to file
            if (module->currentFile && module->fileTransferActive) {
                size_t written = module->currentFile.write((const uint8_t*)value.c_str(), value.length());
                module->transferredBytes += written;
            }
        }
    }
};