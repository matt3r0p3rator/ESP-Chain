#pragma once
#include "module_base.h"
#include "firmware_launcher.h"
#include <vector>

class FirmwareLauncherModule : public Module {
public:
    FirmwareLauncherModule();
    void init() override;
    void loop() override;
    String getName() override;
    String getDescription() override;
    void drawMenu(DisplayManager* display) override;
    bool handleInput(uint8_t button) override;
    const unsigned char* getIcon() override;
    int getIconWidth() override;
    int getIconHeight() override;
    int getIconOffsetY() override;

private:
    enum MenuState {
        MAIN_MENU,
        INSTALL_FROM_URL,
        INSTALL_FROM_SD,
        FIRMWARE_INFO,
        BOOT_SECONDARY,
        ERASE_FIRMWARE,
        WIFI_INPUT,
        URL_INPUT,
        FILE_BROWSER,
        INSTALLING,
        CONFIRM_BOOT,
        CONFIRM_ERASE
    };
    
    MenuState currentState;
    int selectedOption;
    int scrollOffset;
    
    // WiFi credentials
    String wifiSSID;
    String wifiPassword;
    String firmwareURL;
    String selectedFile;
    
    // Installation progress
    size_t installProgress;
    size_t installTotal;
    
    // File browser
    std::vector<String> fileList;
    
    void drawMainMenu(DisplayManager* display);
    void drawInstallFromURL(DisplayManager* display);
    void drawInstallFromSD(DisplayManager* display);
    void drawFirmwareInfo(DisplayManager* display);
    void drawBootSecondary(DisplayManager* display);
    void drawEraseFirmware(DisplayManager* display);
    void drawWiFiInput(DisplayManager* display);
    void drawURLInput(DisplayManager* display);
    void drawFileBrowser(DisplayManager* display);
    void drawInstalling(DisplayManager* display);
    void drawConfirmBoot(DisplayManager* display);
    void drawConfirmErase(DisplayManager* display);
    
    void scanForBinFiles();
    void startInstallation();
    
    static void progressCallback(size_t current, size_t total);
    static FirmwareLauncherModule* activeInstance;
};
