#include "firmware_launcher_module.h"
#include "display_manager.h"
#include "ui/icons.h"
#include <SD.h>

FirmwareLauncherModule* FirmwareLauncherModule::activeInstance = nullptr;

FirmwareLauncherModule::FirmwareLauncherModule() 
    : currentState(MAIN_MENU)
    , selectedOption(0)
    , scrollOffset(0)
    , installProgress(0)
    , installTotal(0) {
}

void FirmwareLauncherModule::init() {
    activeInstance = this;
    FirmwareLauncher::getInstance().setProgressCallback(progressCallback);
    currentState = MAIN_MENU;
    selectedOption = 0;
}

void FirmwareLauncherModule::loop() {
    // Nothing to loop continuously
}

String FirmwareLauncherModule::getName() {
    return "FW Launcher";
}

String FirmwareLauncherModule::getDescription() {
    return "Boot Secondary Firmware";
}

const unsigned char* FirmwareLauncherModule::getIcon() {
    return image_firmware_white_bits;
}

int FirmwareLauncherModule::getIconWidth() {
    return 16;
}

int FirmwareLauncherModule::getIconHeight() {
    return 16;
}

int FirmwareLauncherModule::getIconOffsetY() {
    return 0;
}

void FirmwareLauncherModule::drawMenu(DisplayManager* display) {
    switch (currentState) {
        case MAIN_MENU:
            drawMainMenu(display);
            break;
        case INSTALL_FROM_URL:
            drawInstallFromURL(display);
            break;
        case INSTALL_FROM_SD:
            drawInstallFromSD(display);
            break;
        case FIRMWARE_INFO:
            drawFirmwareInfo(display);
            break;
        case BOOT_SECONDARY:
            drawBootSecondary(display);
            break;
        case ERASE_FIRMWARE:
            drawEraseFirmware(display);
            break;
        case INSTALLING:
            drawInstalling(display);
            break;
        case CONFIRM_BOOT:
            drawConfirmBoot(display);
            break;
        case CONFIRM_ERASE:
            drawConfirmErase(display);
            break;
        case FILE_BROWSER:
            drawFileBrowser(display);
            break;
    }
}

void FirmwareLauncherModule::drawMainMenu(DisplayManager* display) {
    display->clearContent();
    display->drawMenuTitle("Firmware Launcher");
    
    std::vector<String> options;
    bool hasSecondary = FirmwareLauncher::getInstance().hasSecondaryFirmware();
    
    if (hasSecondary) {
        options.push_back("Boot Secondary FW");
        options.push_back("FW Info");
        options.push_back("Erase Secondary FW");
    } else {
        options.push_back("Install from URL");
        options.push_back("Install from SD");
        options.push_back("FW Info");
    }
    
    TFT_eSPI* tft = display->getTFT();
    tft->setTextDatum(TL_DATUM);
    
    int y = 40;
    int itemHeight = 30;
    
    for (size_t i = 0; i < options.size(); i++) {
        if (i == selectedOption) {
            tft->fillRect(10, y - 2, 300, itemHeight - 4, TFT_BLUE);
            tft->setTextColor(TFT_WHITE, TFT_BLUE);
        } else {
            tft->setTextColor(TFT_WHITE, TFT_BLACK);
        }
        
        tft->drawString(options[i], 15, y + 5, 2);
        y += itemHeight;
    }
    
    // Status at bottom
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    if (hasSecondary) {
        tft->drawString("Secondary FW: Installed", 160, 270, 2);
    } else {
        tft->drawString("Secondary FW: None", 160, 270, 2);
    }
}

void FirmwareLauncherModule::drawInstallFromURL(DisplayManager* display) {
    display->clearContent();
    display->drawMenuTitle("Install from URL");
    
    TFT_eSPI* tft = display->getTFT();
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    
    tft->drawString("Feature Coming Soon", 160, 100, 2);
    tft->drawString("Install firmware from", 160, 130, 2);
    tft->drawString("URL (WiFi required)", 160, 150, 2);
    
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    tft->drawString("Press BACK to return", 160, 270, 2);
}

void FirmwareLauncherModule::drawInstallFromSD(DisplayManager* display) {
    if (fileList.empty()) {
        scanForBinFiles();
    }
    
    display->clearContent();
    display->drawMenuTitle("Install from SD");
    
    TFT_eSPI* tft = display->getTFT();
    tft->setTextDatum(TL_DATUM);
    
    if (fileList.empty()) {
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(TFT_YELLOW, TFT_BLACK);
        tft->drawString("No .bin files found", 160, 100, 2);
        tft->drawString("on SD card", 160, 130, 2);
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->drawString("Press BACK to return", 160, 270, 2);
        return;
    }
    
    int y = 40;
    int itemHeight = 30;
    int visibleItems = 7;
    
    for (size_t i = scrollOffset; i < fileList.size() && i < scrollOffset + visibleItems; i++) {
        if (i == selectedOption) {
            tft->fillRect(10, y - 2, 300, itemHeight - 4, TFT_BLUE);
            tft->setTextColor(TFT_WHITE, TFT_BLUE);
        } else {
            tft->setTextColor(TFT_WHITE, TFT_BLACK);
        }
        
        String displayName = fileList[i];
        if (displayName.length() > 35) {
            displayName = displayName.substring(0, 32) + "...";
        }
        
        tft->drawString(displayName, 15, y + 5, 2);
        y += itemHeight;
    }
    
    // Scroll indicator
    if (fileList.size() > visibleItems) {
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(TFT_YELLOW, TFT_BLACK);
        tft->drawString(String(selectedOption + 1) + "/" + String(fileList.size()), 160, 270, 2);
    }
}

void FirmwareLauncherModule::drawFirmwareInfo(DisplayManager* display) {
    display->clearContent();
    display->drawMenuTitle("Firmware Info");
    
    TFT_eSPI* tft = display->getTFT();
    tft->setTextDatum(TL_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    
    String info = FirmwareLauncher::getInstance().getSecondaryFirmwareInfo();
    
    int y = 50;
    int lineStart = 0;
    for (size_t i = 0; i <= info.length(); i++) {
        if (i == info.length() || info[i] == '\n') {
            String line = info.substring(lineStart, i);
            tft->drawString(line, 20, y, 2);
            y += 25;
            lineStart = i + 1;
        }
    }
    
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    tft->drawString("Press BACK to return", 160, 270, 2);
}

void FirmwareLauncherModule::drawBootSecondary(DisplayManager* display) {
    display->clearContent();
    display->drawMenuTitle("Boot Secondary FW");
    
    TFT_eSPI* tft = display->getTFT();
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    
    tft->drawString("Booting into", 160, 80, 2);
    tft->drawString("secondary firmware...", 160, 110, 2);
    
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    tft->drawString("Device will restart", 160, 150, 2);
}

void FirmwareLauncherModule::drawEraseFirmware(DisplayManager* display) {
    display->clearContent();
    display->drawMenuTitle("Erase Secondary FW");
    
    TFT_eSPI* tft = display->getTFT();
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    
    tft->drawString("Erasing secondary", 160, 100, 2);
    tft->drawString("firmware...", 160, 130, 2);
}

void FirmwareLauncherModule::drawInstalling(DisplayManager* display) {
    display->clearContent();
    display->drawMenuTitle("Installing Firmware");
    
    TFT_eSPI* tft = display->getTFT();
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    
    if (installTotal > 0) {
        int progress = (installProgress * 100) / installTotal;
        
        // Progress bar
        int barWidth = 280;
        int barHeight = 30;
        int barX = (320 - barWidth) / 2;
        int barY = 100;
        
        tft->drawRect(barX, barY, barWidth, barHeight, TFT_WHITE);
        int fillWidth = (barWidth - 4) * progress / 100;
        tft->fillRect(barX + 2, barY + 2, fillWidth, barHeight - 4, TFT_GREEN);
        
        // Percentage text
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->drawString(String(progress) + "%", 160, barY + barHeight / 2, 4);
        
        // Size info
        String sizeText = String(installProgress / 1024) + " / " + String(installTotal / 1024) + " KB";
        tft->drawString(sizeText, 160, 150, 2);
    } else {
        tft->drawString("Preparing...", 160, 100, 2);
    }
}

void FirmwareLauncherModule::drawConfirmBoot(DisplayManager* display) {
    display->clearContent();
    display->drawMenuTitle("Confirm Boot");
    
    TFT_eSPI* tft = display->getTFT();
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    
    tft->drawString("Boot into secondary", 160, 70, 2);
    tft->drawString("firmware?", 160, 100, 2);
    
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    tft->drawString("Device will restart", 160, 140, 2);
    
    // Options
    int optionY = 180;
    if (selectedOption == 0) {
        tft->fillRect(60, optionY - 5, 80, 30, TFT_GREEN);
        tft->setTextColor(TFT_BLACK, TFT_GREEN);
        tft->drawString("YES", 100, optionY, 2);
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->drawString("NO", 220, optionY, 2);
    } else {
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->drawString("YES", 100, optionY, 2);
        tft->fillRect(180, optionY - 5, 80, 30, TFT_RED);
        tft->setTextColor(TFT_BLACK, TFT_RED);
        tft->drawString("NO", 220, optionY, 2);
    }
}

void FirmwareLauncherModule::drawConfirmErase(DisplayManager* display) {
    display->clearContent();
    display->drawMenuTitle("Confirm Erase");
    
    TFT_eSPI* tft = display->getTFT();
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    
    tft->drawString("Erase secondary", 160, 70, 2);
    tft->drawString("firmware?", 160, 100, 2);
    
    tft->setTextColor(TFT_RED, TFT_BLACK);
    tft->drawString("This cannot be undone!", 160, 140, 2);
    
    // Options
    int optionY = 180;
    if (selectedOption == 0) {
        tft->fillRect(60, optionY - 5, 80, 30, TFT_GREEN);
        tft->setTextColor(TFT_BLACK, TFT_GREEN);
        tft->drawString("YES", 100, optionY, 2);
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->drawString("NO", 220, optionY, 2);
    } else {
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->drawString("YES", 100, optionY, 2);
        tft->fillRect(180, optionY - 5, 80, 30, TFT_RED);
        tft->setTextColor(TFT_BLACK, TFT_RED);
        tft->drawString("NO", 220, optionY, 2);
    }
}

void FirmwareLauncherModule::drawFileBrowser(DisplayManager* display) {
    drawInstallFromSD(display);
}

bool FirmwareLauncherModule::handleInput(uint8_t button) {
    if (currentState == MAIN_MENU) {
        bool hasSecondary = FirmwareLauncher::getInstance().hasSecondaryFirmware();
        int maxOptions = hasSecondary ? 3 : 3;
        
        if (button == 0) { // Up
            selectedOption = (selectedOption - 1 + maxOptions) % maxOptions;
            return true;
        } else if (button == 1) { // Down
            selectedOption = (selectedOption + 1) % maxOptions;
            return true;
        } else if (button == 2) { // Select
            if (hasSecondary) {
                if (selectedOption == 0) {
                    currentState = CONFIRM_BOOT;
                    selectedOption = 1; // Default to NO
                } else if (selectedOption == 1) {
                    currentState = FIRMWARE_INFO;
                } else if (selectedOption == 2) {
                    currentState = CONFIRM_ERASE;
                    selectedOption = 1; // Default to NO
                }
            } else {
                if (selectedOption == 0) {
                    currentState = INSTALL_FROM_URL;
                } else if (selectedOption == 1) {
                    currentState = INSTALL_FROM_SD;
                    selectedOption = 0;
                    scrollOffset = 0;
                    fileList.clear();
                } else if (selectedOption == 2) {
                    currentState = FIRMWARE_INFO;
                }
            }
            return true;
        }
    } else if (currentState == INSTALL_FROM_SD) {
        if (fileList.empty()) {
            if (button == 3) { // Back
                currentState = MAIN_MENU;
                selectedOption = 0;
                return true;
            }
        } else {
            if (button == 0) { // Up
                if (selectedOption > 0) {
                    selectedOption--;
                    if (selectedOption < scrollOffset) {
                        scrollOffset--;
                    }
                }
                return true;
            } else if (button == 1) { // Down
                if (selectedOption < fileList.size() - 1) {
                    selectedOption++;
                    if (selectedOption >= scrollOffset + 7) {
                        scrollOffset++;
                    }
                }
                return true;
            } else if (button == 2) { // Select
                selectedFile = fileList[selectedOption];
                currentState = INSTALLING;
                installProgress = 0;
                installTotal = 0;
                
                // Start installation in next loop
                delay(100);
                bool success = FirmwareLauncher::getInstance().installFirmwareFromSD(selectedFile);
                
                delay(1000);
                currentState = MAIN_MENU;
                selectedOption = 0;
                fileList.clear();
                return true;
            } else if (button == 3) { // Back
                currentState = MAIN_MENU;
                selectedOption = 0;
                fileList.clear();
                return true;
            }
        }
    } else if (currentState == CONFIRM_BOOT) {
        if (button == 0 || button == 1) { // Up/Down - toggle YES/NO
            selectedOption = 1 - selectedOption;
            return true;
        } else if (button == 2) { // Select
            if (selectedOption == 0) { // YES
                currentState = BOOT_SECONDARY;
                delay(100);
                FirmwareLauncher::getInstance().bootSecondaryFirmware();
            } else { // NO
                currentState = MAIN_MENU;
                selectedOption = 0;
            }
            return true;
        } else if (button == 3) { // Back
            currentState = MAIN_MENU;
            selectedOption = 0;
            return true;
        }
    } else if (currentState == CONFIRM_ERASE) {
        if (button == 0 || button == 1) { // Up/Down - toggle YES/NO
            selectedOption = 1 - selectedOption;
            return true;
        } else if (button == 2) { // Select
            if (selectedOption == 0) { // YES
                currentState = ERASE_FIRMWARE;
                delay(100);
                FirmwareLauncher::getInstance().eraseSecondaryFirmware();
                delay(1000);
                currentState = MAIN_MENU;
                selectedOption = 0;
            } else { // NO
                currentState = MAIN_MENU;
                selectedOption = 0;
            }
            return true;
        } else if (button == 3) { // Back
            currentState = MAIN_MENU;
            selectedOption = 0;
            return true;
        }
    } else if (button == 3) { // Back from other states
        currentState = MAIN_MENU;
        selectedOption = 0;
        fileList.clear();
        return true;
    }
    
    return false;
}

void FirmwareLauncherModule::scanForBinFiles() {
    fileList.clear();
    
    File root = SD.open("/");
    if (!root) {
        Serial.println("Failed to open root directory");
        return;
    }
    
    File file = root.openNextFile();
    while (file) {
        String fileName = String(file.name());
        if (!file.isDirectory() && fileName.endsWith(".bin")) {
            fileList.push_back("/" + fileName);
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();
    
    // Also check /firmware folder if it exists
    if (SD.exists("/firmware")) {
        File fwFolder = SD.open("/firmware");
        if (fwFolder) {
            file = fwFolder.openNextFile();
            while (file) {
                String fileName = String(file.name());
                if (!file.isDirectory() && fileName.endsWith(".bin")) {
                    fileList.push_back("/firmware/" + fileName);
                }
                file.close();
                file = fwFolder.openNextFile();
            }
            fwFolder.close();
        }
    }
    
    Serial.printf("Found %d .bin files\n", fileList.size());
}

void FirmwareLauncherModule::progressCallback(size_t current, size_t total) {
    if (activeInstance) {
        activeInstance->installProgress = current;
        activeInstance->installTotal = total;
    }
}
