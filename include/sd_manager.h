#pragma once
#include <FS.h>
#include <SD.h>
#include <SPI.h>

class SDManager {
public:
    bool init();
    String readFile(String path);
    bool writeFile(String path, String content);
};
