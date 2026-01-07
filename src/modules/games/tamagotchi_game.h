#pragma once
#include <Arduino.h>
#include "module_base.h"
#include "display_manager.h"
#include "config_manager.h"

enum PetState { HAPPY, HUNGRY, BORED, SLEEPY, SICK, DEAD };
enum PetMood { VERY_HAPPY, CONTENT, NEUTRAL, SAD, VERY_SAD };

class TamagotchiGame : public Module {
private:
    // Pet stats
    int hunger;           // 0-100 (0 = full, 100 = starving)
    int happiness;        // 0-100 (0 = sad, 100 = very happy)
    int energy;          // 0-100 (0 = exhausted, 100 = full energy)
    int health;          // 0-100 (0 = dead, 100 = perfect health)
    int age;             // In seconds
    int weight;          // 10-50 kg
    bool isAlive;
    bool isSleeping;
    
    // Time tracking
    unsigned long lastUpdateTime;
    unsigned long birthTime;
    unsigned long sleepStartTime;
    
    // Menu
    int selectedAction;
    const char* actions[6] = {"Feed", "Play", "Clean", "Sleep", "Medicine", "Stats"};
    int numActions = 6;
    
    // Pet appearance animation
    int animFrame;
    unsigned long lastAnimTime;
    
    // Poop tracking
    int poopCount;
    unsigned long lastPoopTime;
    
    // Evolution stages
    enum Stage { EGG, BABY, CHILD, TEEN, ADULT } stage;
    
    PetState getCurrentState() {
        if (!isAlive) return DEAD;
        if (health < 30) return SICK;
        if (isSleeping) return SLEEPY;
        if (hunger > 70) return HUNGRY;
        if (happiness < 30) return BORED;
        return HAPPY;
    }
    
    PetMood getMood() {
        int avgStat = (happiness + (100 - hunger) + energy + health) / 4;
        if (avgStat > 80) return VERY_HAPPY;
        if (avgStat > 60) return CONTENT;
        if (avgStat > 40) return NEUTRAL;
        if (avgStat > 20) return SAD;
        return VERY_SAD;
    }
    
    void updateStats() {
        unsigned long currentTime = millis();
        unsigned long deltaTime = currentTime - lastUpdateTime;
        
        if (deltaTime < 1000) return; // Update every second
        
        int seconds = deltaTime / 1000;
        lastUpdateTime = currentTime;
        
        if (!isAlive) return;
        
        // Age the pet
        age += seconds;
        
        // Update evolution stage
        if (age < 60) stage = EGG;
        else if (age < 180) stage = BABY;
        else if (age < 360) stage = CHILD;
        else if (age < 600) stage = TEEN;
        else stage = ADULT;
        
        // Stats naturally decrease over time
        if (!isSleeping) {
            hunger += seconds * 1;  // Gets hungry
            happiness -= seconds * 1; // Gets bored
            energy -= seconds * 2;   // Gets tired
            
            // Generate poop
            if (currentTime - lastPoopTime > 30000 && poopCount < 5) { // Every 30 seconds
                poopCount++;
                lastPoopTime = currentTime;
                happiness -= 5;
            }
            
            // Poop affects health
            if (poopCount > 2) {
                health -= seconds * 1;
            }
        } else {
            // Sleeping restores energy
            energy += seconds * 5;
            if (energy >= 100) {
                energy = 100;
                isSleeping = false;
            }
        }
        
        // Extreme hunger affects health
        if (hunger > 80) {
            health -= seconds * 2;
        }
        
        // Very low happiness affects health
        if (happiness < 20) {
            health -= seconds * 1;
        }
        
        // Clamp values
        hunger = constrain(hunger, 0, 100);
        happiness = constrain(happiness, 0, 100);
        energy = constrain(energy, 0, 100);
        health = constrain(health, 0, 100);
        
        // Check if pet died
        if (health <= 0) {
            isAlive = false;
            health = 0;
        }
    }
    
    void feed() {
        if (!isAlive || isSleeping) return;
        hunger -= 30;
        if (hunger < 0) hunger = 0;
        weight += 1;
        if (weight > 50) weight = 50;
        happiness += 5;
        if (happiness > 100) happiness = 100;
    }
    
    void play() {
        if (!isAlive || isSleeping) return;
        if (energy < 20) return; // Too tired to play
        happiness += 25;
        if (happiness > 100) happiness = 100;
        energy -= 20;
        hunger += 10;
    }
    
    void clean() {
        if (!isAlive) return;
        if (poopCount > 0) {
            poopCount = 0;
            happiness += 10;
            if (happiness > 100) happiness = 100;
            health += 10;
            if (health > 100) health = 100;
        }
    }
    
    void sleep() {
        if (!isAlive) return;
        if (energy < 50) {
            isSleeping = true;
            sleepStartTime = millis();
        }
    }
    
    void giveMedicine() {
        if (!isAlive) return;
        health += 40;
        if (health > 100) health = 100;
        happiness -= 10; // Doesn't like medicine
    }
    
    void drawPet(TFT_eSPI* tft, int x, int y) {
        PetState state = getCurrentState();
        
        // Draw pet based on stage and state
        int size = 40 + (stage * 8);
        uint16_t color;
        
        switch (getMood()) {
            case VERY_HAPPY: color = TFT_GREEN; break;
            case CONTENT: color = TFT_CYAN; break;
            case NEUTRAL: color = TFT_YELLOW; break;
            case SAD: color = TFT_ORANGE; break;
            case VERY_SAD: color = TFT_RED; break;
        }
        
        if (!isAlive) {
            // Dead pet
            tft->fillCircle(x, y, size/2, TFT_DARKGREY);
            tft->drawLine(x - 8, y - 8, x - 2, y - 2, TFT_BLACK);
            tft->drawLine(x - 2, y - 8, x - 8, y - 2, TFT_BLACK);
            tft->drawLine(x + 2, y - 8, x + 8, y - 2, TFT_BLACK);
            tft->drawLine(x + 8, y - 8, x + 2, y - 2, TFT_BLACK);
            return;
        }
        
        // Body
        tft->fillCircle(x, y, size/2, color);
        
        // Eyes
        if (isSleeping) {
            tft->drawLine(x - 10, y - 8, x - 4, y - 8, TFT_BLACK);
            tft->drawLine(x + 4, y - 8, x + 10, y - 8, TFT_BLACK);
        } else {
            int eyeSize = (animFrame % 30 < 28) ? 3 : 1; // Blink
            tft->fillCircle(x - 8, y - 8, eyeSize, TFT_BLACK);
            tft->fillCircle(x + 8, y - 8, eyeSize, TFT_BLACK);
        }
        
        // Mouth based on mood
        if (state == SICK) {
            tft->drawLine(x - 6, y + 8, x + 6, y + 8, TFT_BLACK);
        } else if (getMood() == VERY_HAPPY || getMood() == CONTENT) {
            // Smile
            for (int i = -8; i <= 8; i++) {
                int yOffset = y + 5 + (abs(i) / 3);
                tft->drawPixel(x + i, yOffset, TFT_BLACK);
            }
        } else if (getMood() == SAD || getMood() == VERY_SAD) {
            // Frown
            for (int i = -8; i <= 8; i++) {
                int yOffset = y + 12 - (abs(i) / 3);
                tft->drawPixel(x + i, yOffset, TFT_BLACK);
            }
        } else {
            // Neutral
            tft->drawLine(x - 8, y + 8, x + 8, y + 8, TFT_BLACK);
        }
        
        // Stage indicator (decoration)
        if (stage >= CHILD) {
            // Draw ears/horns for older pets
            tft->fillTriangle(x - size/2 - 5, y - 10, x - size/2, y - 20, x - size/2 + 5, y - 10, color);
            tft->fillTriangle(x + size/2 - 5, y - 10, x + size/2, y - 20, x + size/2 + 5, y - 10, color);
        }
        
        // Draw poop
        for (int i = 0; i < poopCount && i < 5; i++) {
            int px = x - 50 + (i * 15);
            int py = y + 30;
            tft->fillCircle(px, py, 5, TFT_BROWN);
            tft->fillCircle(px, py - 5, 4, TFT_BROWN);
            tft->fillCircle(px, py - 8, 3, TFT_BROWN);
        }
        
        // Sick indicator
        if (state == SICK) {
            tft->setTextColor(TFT_PURPLE, TFT_BLACK);
            tft->setTextDatum(MC_DATUM);
            tft->drawString("SICK!", x, y - size/2 - 15, 2);
        }
    }
    
    void drawStats(TFT_eSPI* tft) {
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->setTextDatum(TL_DATUM);
        
        int barWidth = 120;
        int barHeight = 10;
        int startX = 10;
        int startY = 30;
        int spacing = 18;
        
        // Hunger bar
        tft->drawString("Hunger:", startX, startY, 2);
        tft->drawRect(startX + 80, startY, barWidth, barHeight, TFT_WHITE);
        int hungerWidth = map(100 - hunger, 0, 100, 0, barWidth - 2);
        tft->fillRect(startX + 81, startY + 1, hungerWidth, barHeight - 2, TFT_GREEN);
        
        // Happiness bar
        tft->drawString("Happy:", startX, startY + spacing, 2);
        tft->drawRect(startX + 80, startY + spacing, barWidth, barHeight, TFT_WHITE);
        int happyWidth = map(happiness, 0, 100, 0, barWidth - 2);
        tft->fillRect(startX + 81, startY + spacing + 1, happyWidth, barHeight - 2, TFT_YELLOW);
        
        // Energy bar
        tft->drawString("Energy:", startX, startY + spacing * 2, 2);
        tft->drawRect(startX + 80, startY + spacing * 2, barWidth, barHeight, TFT_WHITE);
        int energyWidth = map(energy, 0, 100, 0, barWidth - 2);
        tft->fillRect(startX + 81, startY + spacing * 2 + 1, energyWidth, barHeight - 2, TFT_CYAN);
        
        // Health bar
        tft->drawString("Health:", startX, startY + spacing * 3, 2);
        tft->drawRect(startX + 80, startY + spacing * 3, barWidth, barHeight, TFT_WHITE);
        int healthWidth = map(health, 0, 100, 0, barWidth - 2);
        uint16_t healthColor = health > 60 ? TFT_GREEN : (health > 30 ? TFT_ORANGE : TFT_RED);
        tft->fillRect(startX + 81, startY + spacing * 3 + 1, healthWidth, barHeight - 2, healthColor);
        
        // Age and weight
        tft->drawString("Age: " + String(age) + "s", startX, startY + spacing * 4 + 5, 2);
        tft->drawString("Weight: " + String(weight) + "kg", startX, startY + spacing * 5, 2);
        
        // Stage
        String stageName;
        switch (stage) {
            case EGG: stageName = "Egg"; break;
            case BABY: stageName = "Baby"; break;
            case CHILD: stageName = "Child"; break;
            case TEEN: stageName = "Teen"; break;
            case ADULT: stageName = "Adult"; break;
        }
        tft->drawString("Stage: " + stageName, startX, startY + spacing * 6 - 5, 2);
    }
    
    void drawActions(TFT_eSPI* tft) {
        tft->setTextDatum(TL_DATUM);
        int startY = 135;
        int itemWidth = 50;
        
        for (int i = 0; i < numActions; i++) {
            int x = 5 + (i % 3) * (itemWidth + 50);
            int y = startY + (i / 3) * 20;
            
            if (i == selectedAction) {
                tft->setTextColor(TFT_BLACK, TFT_WHITE);
                tft->fillRect(x - 2, y - 2, itemWidth + 40, 18, TFT_WHITE);
            } else {
                tft->setTextColor(TFT_WHITE, TFT_BLACK);
            }
            
            tft->drawString(actions[i], x, y, 2);
        }
    }

public:
    TamagotchiGame() {}

    void init() override {
        // Initialize pet
        hunger = 30;
        happiness = 80;
        energy = 100;
        health = 100;
        age = 0;
        weight = 20;
        isAlive = true;
        isSleeping = false;
        poopCount = 0;
        stage = EGG;
        
        selectedAction = 0;
        animFrame = 0;
        
        lastUpdateTime = millis();
        birthTime = millis();
        lastPoopTime = millis();
        lastAnimTime = millis();
    }

    void loop() override {
        updateStats();
        
        // Update animation
        if (millis() - lastAnimTime > 100) {
            lastAnimTime = millis();
            animFrame++;
        }
    }

    String getName() override {
        return "Tamagotchi";
    }

    String getDescription() override {
        return "Virtual Pet Game";
    }

    void drawMenu(DisplayManager* display) override {
        display->clearContent();
        display->drawMenuTitle("Tamagotchi");
        
        extern DisplayManager displayManager;
        TFT_eSPI* tft = displayManager.getTFT();
        
        // Draw pet in center
        drawPet(tft, 260, 60);
        
        // Draw stats on left
        drawStats(tft);
        
        // Draw actions at bottom
        drawActions(tft);
        
        // Draw status message
        if (!isAlive) {
            tft->setTextColor(TFT_RED, TFT_BLACK);
            tft->setTextDatum(MC_DATUM);
            tft->drawString("R.I.P.", 260, 100, 2);
        } else if (isSleeping) {
            tft->setTextColor(TFT_CYAN, TFT_BLACK);
            tft->setTextDatum(MC_DATUM);
            tft->drawString("Zzz...", 260, 100, 2);
        }
    }

    bool handleInput(uint8_t button) override {
        if (button == 3) return false; // Back -> Exit
        
        extern DisplayManager displayManager;
        
        switch (button) {
            case 0: // Up/Left - Previous action
                selectedAction--;
                if (selectedAction < 0) selectedAction = numActions - 1;
                drawMenu(&displayManager);
                break;
                
            case 1: // Down/Right - Next action
                selectedAction++;
                if (selectedAction >= numActions) selectedAction = 0;
                drawMenu(&displayManager);
                break;
                
            case 2: // Select - Execute action
                switch (selectedAction) {
                    case 0: feed(); break;
                    case 1: play(); break;
                    case 2: clean(); break;
                    case 3: sleep(); break;
                    case 4: giveMedicine(); break;
                    case 5: /* Stats - already shown */ break;
                }
                
                // Check if pet died after action
                if (!isAlive) {
                    drawMenu(&displayManager);
                    delay(500);
                }
                drawMenu(&displayManager);
                break;
        }
        
        return true;
    }
};
