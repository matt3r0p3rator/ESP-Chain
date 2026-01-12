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
    , installTotal(0)
    , bootStartTime(0)
    , bootTriggered(false) {
}

void FirmwareLauncherModule::init() {
    activeInstance = this;
    FirmwareLauncher::getInstance().setProgressCallback(progressCallback);
    currentState = MAIN_MENU;
    selectedOption = 0;
}

void FirmwareLauncherModule::loop() {
    // Handle delayed boot to secondary firmware
    if (currentState == BOOT_SECONDARY && !bootTriggered) {
        if (bootStartTime == 0) {
            bootStartTime = millis();
            Serial.println("Boot screen displayed, waiting 500ms...");
        } else if (millis() - bootStartTime > 500) {
            // Give time for the boot screen to display
            bootTriggered = true;
            Serial.println("Attempting to boot secondary firmware...");
            
            if (!FirmwareLauncher::getInstance().bootSecondaryFirmware()) {
                // If boot failed, show error
                Serial.println("Boot failed - no valid firmware or error");
                errorMessage = "No Firmware Installed!\n\nUse 'Install from SD'\nto load a .bin file\nfirst\n\nPress any button";
                currentState = BOOT_ERROR;
                bootStartTime = 0;
                bootTriggered = false;
            }
            // If boot succeeds, ESP.restart() is called and we never reach here
        }
    }
    
    // Handle delayed erase
    if (currentState == ERASE_FIRMWARE) {
        if (FirmwareLauncher::getInstance().eraseSecondaryFirmware()) {
            Serial.println("Secondary firmware erased successfully");
        } else {
            Serial.println("Failed to erase secondary firmware");
        }
        delay(1000);
        currentState = MAIN_MENU;
        selectedOption = 0;
    }
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
        case BOOT_ERROR:
            drawBootError(display);
            break;
    }
}

void FirmwareLauncherModule::drawMainMenu(DisplayManager* display) {
    display->clearMenu();
    
    bool hasSecondary = FirmwareLauncher::getInstance().hasSecondaryFirmware();
    
    String items[4];
    int totalItems;
    
    if (hasSecondary) {
        items[0] = "Boot Secondary FW";
        items[1] = "FW Info";
        items[2] = "Erase Secondary FW";
        totalItems = 3;
    } else {
        items[0] = "Install from SD";
        items[1] = "Install from URL";
        items[2] = "FW Info";
        totalItems = 3;
    }
    
    int start = scrollOffset;
    int end = start + itemsPerPage;
    if (end > totalItems) end = totalItems;
    
    for (int i = start; i < end; i++) {
        display->drawMenuItem(items[i], i - scrollOffset, i == selectedOption);
    }
    
    display->drawScrollBar(totalItems, scrollOffset, itemsPerPage);
    display->updateMenu();
    
    // Status at bottom
    TFT_eSPI* tft = display->getTFT();
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    if (hasSecondary) {
        tft->drawString("Secondary FW: Ready", 160, 270, 2);
    } else {
        tft->drawString("Install firmware to boot", 160, 270, 2);
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
    
    display->clearMenu();
    
    TFT_eSPI* tft = display->getTFT();
    
    if (fileList.empty()) {
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(TFT_YELLOW, TFT_BLACK);
        tft->drawString("No .bin files found", 160, 100, 2);
        tft->drawString("on SD card", 160, 130, 2);
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->drawString("Press BACK to return", 160, 270, 2);
        return;
    }
    
    int totalItems = fileList.size();
    int start = scrollOffset;
    int end = start + itemsPerPage;
    if (end > totalItems) end = totalItems;
    
    for (int i = start; i < end; i++) {
        String displayName = fileList[i];
        if (displayName.length() > 35) {
            displayName = displayName.substring(0, 32) + "...";
        }
        display->drawMenuItem(displayName, i - scrollOffset, i == selectedOption);
    }
    
    display->drawScrollBar(totalItems, scrollOffset, itemsPerPage);
    display->updateMenu();
    
    // File count indicator
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    tft->drawString(String(selectedOption + 1) + "/" + String(totalItems), 160, 270, 2);
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
    display->clearMenu();
    
    String items[] = {"YES - Boot Secondary FW", "NO - Cancel"};
    int totalItems = 2;
    
    for (int i = 0; i < totalItems; i++) {
        display->drawMenuItem(items[i], i, i == selectedOption);
    }
    
    display->updateMenu();
    
    TFT_eSPI* tft = display->getTFT();
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    tft->drawString("Device will restart", 160, 160, 2);
}

void FirmwareLauncherModule::drawConfirmErase(DisplayManager* display) {
    display->clearMenu();
    
    String items[] = {"YES - Erase Firmware", "NO - Cancel"};
    int totalItems = 2;
    
    for (int i = 0; i < totalItems; i++) {
        display->drawMenuItem(items[i], i, i == selectedOption);
    }
    
    display->updateMenu();
    
    TFT_eSPI* tft = display->getTFT();
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_RED, TFT_BLACK);
    tft->drawString("This cannot be undone!", 160, 160, 2);
}

void FirmwareLauncherModule::drawBootError(DisplayManager* display) {
    display->clearContent();
    display->drawMenuTitle("Boot Error");
    
    TFT_eSPI* tft = display->getTFT();
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_RED, TFT_BLACK);
    
    int y = 70;
    int lineStart = 0;
    for (size_t i = 0; i <= errorMessage.length(); i++) {
        if (i == errorMessage.length() || errorMessage[i] == '\n') {
            String line = errorMessage.substring(lineStart, i);
            tft->drawString(line, 160, y, 2);
            y += 25;
            lineStart = i + 1;
        }
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
            if (selectedOption > 0) {
                selectedOption--;
                if (selectedOption < scrollOffset) {
                    scrollOffset--;
                }
            } else {
                selectedOption = maxOptions - 1;
                scrollOffset = maxOptions - itemsPerPage;
                if (scrollOffset < 0) scrollOffset = 0;
            }
            return true;
        } else if (button == 1) { // Down
            if (selectedOption < maxOptions - 1) {
                selectedOption++;
                if (selectedOption >= scrollOffset + itemsPerPage) {
                    scrollOffset++;
                }
            } else {
                selectedOption = 0;
                scrollOffset = 0;
            }
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
                    currentState = INSTALL_FROM_SD;
                    selectedOption = 0;
                    scrollOffset = 0;
                    fileList.clear();
                } else if (selectedOption == 1) {
                    currentState = INSTALL_FROM_URL;
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
                    if (selectedOption >= scrollOffset + itemsPerPage) {
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
        if (button == 0) { // Up
            selectedOption = (selectedOption == 0) ? 1 : 0;
            return true;
        } else if (button == 1) { // Down
            selectedOption = (selectedOption == 0) ? 1 : 0;
            return true;
        } else if (button == 2) { // Select
            if (selectedOption == 0) { // YES
                Serial.println("User confirmed boot to secondary firmware");
                currentState = BOOT_SECONDARY;
                bootStartTime = 0;
                bootTriggered = false;
                return true; // Draw the boot screen first
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
        if (button == 0) { // Up
            selectedOption = (selectedOption == 0) ? 1 : 0;
            return true;
        } else if (button == 1) { // Down
            selectedOption = (selectedOption == 0) ? 1 : 0;
            return true;
        } else if (button == 2) { // Select
            if (selectedOption == 0) { // YES
                Serial.println("User confirmed erase secondary firmware");
                currentState = ERASE_FIRMWARE;
                return true;
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
    } else if (currentState == BOOT_ERROR) {
        // Any button press returns to main menu
        currentState = MAIN_MENU;
        selectedOption = 0;
        return true;
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
