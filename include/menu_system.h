#pragma once
#include <vector>
#include "module_base.h"
#include "display_manager.h"

class MenuSystem {
public:
    MenuSystem(DisplayManager* display);
    void registerModule(Module* module);
    void draw();
    void handleInput(uint8_t input); // 0=Up, 1=Down, 2=Select, 3=Back
    void update();

private:
    DisplayManager* displayManager;
    std::vector<Module*> modules;
    int selectedIndex;
    bool inModule;
    Module* activeModule;
};
