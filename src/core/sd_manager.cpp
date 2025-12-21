#include "sd_manager.h"

// SD Card Pins
#define SD_CS   10
#define SD_MOSI 11
#define SD_SCK  12
#define SD_MISO 13

bool SDManager::init() {
    // Initialize SPI with defined pins
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    
    // Initialize SD Card
    if (!SD.begin(SD_CS)) {
        isSDMounted = false;
        return false;
    }
    
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        isSDMounted = false;
        return false;
    }
    
    isSDMounted = true;
    return true;
}

bool SDManager::isMounted() {
    return isSDMounted;
}

String SDManager::readFile(String path) {
    if (!SD.exists(path)) return "";
    
    File file = SD.open(path);
    if (!file) return "";
    
    String content = file.readString();
    file.close();
    return content;
}

bool SDManager::writeFile(String path, String content) {
    File file = SD.open(path, FILE_WRITE);
    if (!file) return false;
    
    if (file.print(content)) {
        file.close();
        return true;
    }
    
    file.close();
    return false;
}

std::vector<FileEntry> SDManager::listDir(String path) {
    std::vector<FileEntry> fileList;
    File root = SD.open(path);
    if (!root || !root.isDirectory()) {
        return fileList;
    }

    File file = root.openNextFile();
    while (file) {
        FileEntry entry;
        entry.name = String(file.name());
        entry.isDirectory = file.isDirectory();
        entry.size = file.size();
        fileList.push_back(entry);
        file = root.openNextFile();
    }
    return fileList;
}
