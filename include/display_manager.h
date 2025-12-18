#pragma once
#include <TFT_eSPI.h>

class DisplayManager {
public:
    DisplayManager();
    void init();
    void clear();
    void drawStatusBar(String status, int batteryLevel);
    void drawMenuTitle(String title);
    TFT_eSPI& getTFT();

private:
    TFT_eSPI* tft;
};
