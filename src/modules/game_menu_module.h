#pragma once
#include <vector>
#include "module_base.h"
#include "games/snake_game.h"

class GameMenuModule : public Module {
private:
    std::vector<Module*> games;
    Module* activeGame;
    int selectedIndex;
    bool inGame;

    SnakeGame snakeGame;

public:
    GameMenuModule() : activeGame(nullptr), selectedIndex(0), inGame(false) {
        games.push_back(&snakeGame);
    }

    void init() override {
        inGame = false;
        activeGame = nullptr;
        selectedIndex = 0;
    }

    void loop() override {
        if (inGame && activeGame) {
            activeGame->loop();
        }
    }

    String getName() override {
        return "Games";
    }

    String getDescription() override {
        return "Play Games";
    }

    const unsigned char* getIcon() override { 
        // Use a generic icon or nullptr for now
        return nullptr; 
    }

    void drawMenu(DisplayManager* display) override {
        if (inGame && activeGame) {
            activeGame->drawMenu(display);
            return;
        }

        display->clearContent();
        display->drawMenuTitle("Games");

        for (int i = 0; i < games.size(); i++) {
            display->drawMenuItem(games[i]->getName(), i, i == selectedIndex, games[i]->getIcon(), games[i]->getIconWidth(), games[i]->getIconHeight(), games[i]->getIconSpacing(), games[i]->getIconOffsetY());
        }
    }

    bool handleInput(uint8_t button) override {
        if (inGame && activeGame) {
            if (!activeGame->handleInput(button)) {
                // Game exited
                inGame = false;
                activeGame = nullptr;
                // Redraw menu
                extern DisplayManager displayManager;
                drawMenu(&displayManager);
            }
            return true;
        }

        // Menu Navigation
        switch (button) {
            case 0: // Up
                if (selectedIndex > 0) selectedIndex--;
                else selectedIndex = games.size() - 1;
                {
                    extern DisplayManager displayManager;
                    drawMenu(&displayManager);
                }
                break;
            case 1: // Down
                if (selectedIndex < games.size() - 1) selectedIndex++;
                else selectedIndex = 0;
                {
                    extern DisplayManager displayManager;
                    drawMenu(&displayManager);
                }
                break;
            case 2: // Select
                if (games.size() > 0) {
                    activeGame = games[selectedIndex];
                    inGame = true;
                    activeGame->init();
                    extern DisplayManager displayManager;
                    activeGame->drawMenu(&displayManager);
                }
                break;
            case 3: // Back
                return false; // Exit Game Menu
        }
        return true;
    }
};
