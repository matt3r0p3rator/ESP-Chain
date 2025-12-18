#include "menu_system.h"

MenuSystem::MenuSystem(DisplayManager* display) : displayManager(display), selectedIndex(0), inModule(false), activeModule(nullptr) {}

void MenuSystem::registerModule(Module* module) {
    modules.push_back(module);
}

void MenuSystem::draw() {
    if (inModule && activeModule) {
        activeModule->drawMenu(displayManager->getTFT());
        return;
    }

    displayManager->clear();
    displayManager->drawStatusBar("Menu", 100); 
    displayManager->drawMenuTitle("ESP-Chain");

    int y = 60;
    for (size_t i = 0; i < modules.size(); i++) {
        if ((int)i == selectedIndex) {
            displayManager->getTFT().setTextColor(TFT_BLACK, TFT_GREEN);
        } else {
            displayManager->getTFT().setTextColor(TFT_GREEN, TFT_BLACK);
        }
        displayManager->getTFT().setCursor(20, y);
        displayManager->getTFT().print(modules[i]->getName());
        y += 30;
    }
}

void MenuSystem::handleInput(uint8_t input) {
    if (inModule && activeModule) {
        if (input == 3) {
            inModule = false;
            activeModule = nullptr;
            draw();
        } else {
            activeModule->handleInput(input);
        }
        return;
    }

    switch (input) {
        case 0: // Up
            if (selectedIndex > 0) selectedIndex--;
            break;
        case 1: // Down
            if (selectedIndex < (int)modules.size() - 1) selectedIndex++;
            break;
        case 2: // Select
            if (selectedIndex >= 0 && selectedIndex < (int)modules.size()) {
                activeModule = modules[selectedIndex];
                inModule = true;
                activeModule->init(); 
                draw(); 
            }
            break;
    }
    draw();
}

void MenuSystem::update() {
    if (inModule && activeModule) {
        activeModule->loop();
    }
}
