#include "sd_manager.h"

bool SDManager::init() {
    // Try default begin
    if (!SD.begin()) {
        return false;
    }
    return true;
}

String SDManager::readFile(String path) {
    File file = SD.open(path);
    if (!file) return "";
    String content = file.readString();
    file.close();
    return content;
}

bool SDManager::writeFile(String path, String content) {
    File file = SD.open(path, FILE_WRITE);
    if (!file) return false;
    file.print(content);
    file.close();
    return true;
}
