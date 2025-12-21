#pragma once
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <vector>

struct FileEntry {
    String name;
    bool isDirectory;
    size_t size;
};

class SDManager {
public:
    bool init();
    bool isMounted();
    String readFile(String path);
    bool writeFile(String path, String content);
    std::vector<FileEntry> listDir(String path);

private:
    bool isSDMounted = false;
};
