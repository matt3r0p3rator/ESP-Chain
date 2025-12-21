#pragma once
#include <vector>
#include "module_base.h"
#include "display_manager.h"
#include "sd_manager.h"

class MenuSystem {
public:
    MenuSystem(DisplayManager* display, SDManager* sd);
    void registerModule(Module* module);
    void draw();
    void handleInput(uint8_t input); // 0=Up, 1=Down, 2=Select, 3=Back
    void update();

private:
    void enterDeepSleep();

    DisplayManager* displayManager;
    SDManager* sdManager;
    std::vector<Module*> modules;
    int selectedIndex;
    int scrollOffset;
    const int itemsPerPage = 5;
    bool inModule;
    Module* activeModule;

    bool isDeepSleepPending;
    unsigned long deepSleepStartTime;
};
