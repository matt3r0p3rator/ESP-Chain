#pragma once
#include "module_base.h"
#include "sd_manager.h"
#include "display_manager.h"
#include <vector>

class FileExplorerModule : public Module {
private:
    enum State { BROWSER, VIEWER };
    State currentState;

    // Browser State
    String currentPath;
    std::vector<FileEntry> currentFiles;
    int selectedIndex;
    int scrollOffset;

    // Viewer State
    std::vector<String> viewerLines;
    int viewerScrollIndex;

    void loadPath(String path) {
        extern SDManager sdManager;
        currentPath = path;
        currentFiles = sdManager.listDir(currentPath);
        selectedIndex = 0;
        scrollOffset = 0;
        currentState = BROWSER;
    }

    void openFile(String path) {
        extern SDManager sdManager;
        String content = sdManager.readFile(path);
        
        viewerLines.clear();
        // Split by newline
        int start = 0;
        while (start < content.length()) {
            int end = content.indexOf('\n', start);
            if (end == -1) end = content.length();
            
            String line = content.substring(start, end);
            line.trim(); // Remove \r
            
            viewerLines.push_back(line);
            
            start = end + 1;
        }
        
        if (viewerLines.empty()) viewerLines.push_back("<Empty File>");
        
        viewerScrollIndex = 0;
        currentState = VIEWER;
    }

public:
    void init() override {
        loadPath("/");
    }

    void loop() override {}

    String getName() override {
        return "File Explorer";
    }

    String getDescription() override {
        return "Browse SD Card";
    }

    void drawMenu(DisplayManager* display) override {
        display->clearContent();
        
        if (currentState == VIEWER) {
            display->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
            display->getTFT()->setTextDatum(TL_DATUM);
            
            int linesPerPage = 6; // Fits in 150px height (20px per line)
            for (int i = 0; i < linesPerPage; i++) {
                int idx = viewerScrollIndex + i;
                if (idx >= viewerLines.size()) break;
                display->getTFT()->drawString(viewerLines[idx], 10, 30 + (i * 20), 2);
            }
            
            // Scroll Indicator
            if (viewerLines.size() > linesPerPage) {
                String scrollInfo = String(viewerScrollIndex + 1) + "/" + String(viewerLines.size());
                display->getTFT()->setTextDatum(TR_DATUM);
                display->getTFT()->drawString(scrollInfo, 310, 30, 2);
            }
            return;
        }

        // BROWSER MODE
        if (currentFiles.empty()) {
             display->getTFT()->setTextDatum(MC_DATUM);
             display->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
             display->getTFT()->drawString("Empty Folder", 160, 100, 2);
             return;
        }

        for (int i = 0; i < 5; i++) {
            int idx = scrollOffset + i;
            if (idx >= currentFiles.size()) break;
            
            String label = currentFiles[idx].name;
            if (currentFiles[idx].isDirectory) label += "/";
            else label = " " + label; 

            display->drawMenuItem(label, i, idx == selectedIndex);
        }
    }

    bool handleInput(uint8_t button) override {
        extern DisplayManager displayManager;

        if (currentState == VIEWER) {
            if (button == 3) { // Back (Long Press)
                currentState = BROWSER;
                drawMenu(&displayManager);
                return true;
            }
            if (button == 1) { // Scroll Down (Single Click)
                viewerScrollIndex++;
                if (viewerScrollIndex >= viewerLines.size()) viewerScrollIndex = 0;
                drawMenu(&displayManager);
                return true;
            }
            if (button == 2) { // Page Down / Fast Scroll (Double Click)
                viewerScrollIndex += 5;
                if (viewerScrollIndex >= viewerLines.size()) viewerScrollIndex = 0;
                drawMenu(&displayManager);
                return true;
            }
            return true;
        }

        // BROWSER INPUT
        if (button == 1) { // Scroll
            if (currentFiles.empty()) return true;
            selectedIndex++;
            if (selectedIndex >= currentFiles.size()) {
                selectedIndex = 0;
                scrollOffset = 0;
            } else if (selectedIndex >= scrollOffset + 5) {
                scrollOffset++;
            }
            drawMenu(&displayManager);
            return true;
        }

        if (button == 2) { // Select
            if (currentFiles.empty()) return true;
            
            FileEntry& entry = currentFiles[selectedIndex];
            String newPath = currentPath;
            if (!newPath.endsWith("/")) newPath += "/";
            newPath += entry.name;

            if (entry.isDirectory) {
                loadPath(newPath);
            } else {
                openFile(newPath);
            }
            drawMenu(&displayManager);
            return true;
        }

        if (button == 3) { // Back
            if (currentPath == "/") {
                return false; // Exit module
            } else {
                String temp = currentPath;
                if (temp.endsWith("/") && temp.length() > 1) temp.remove(temp.length() - 1);
                int lastSlash = temp.lastIndexOf('/');
                if (lastSlash <= 0) loadPath("/");
                else loadPath(temp.substring(0, lastSlash + 1));
                drawMenu(&displayManager);
                return true;
            }
        }
        return true;
    }
};
