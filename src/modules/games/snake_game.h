#pragma once
#include <Arduino.h>
#include <vector>
#include "module_base.h"
#include "display_manager.h"

struct Point {
    int x, y;
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

enum Direction { UP, DOWN, LEFT, RIGHT };

class SnakeGame : public Module {
private:
    std::vector<Point> snake;
    Point food;
    Direction dir;
    Direction nextDir;
    bool gameOver;
    bool paused;
    int score;
    unsigned long lastMoveTime;
    int moveInterval;
    int gridWidth;
    int gridHeight;
    int cellSize;
    int offsetX;
    int offsetY;

    void spawnFood() {
        bool onSnake = true;
        while (onSnake) {
            food.x = random(0, gridWidth);
            food.y = random(0, gridHeight);
            onSnake = false;
            for (const auto& p : snake) {
                if (p == food) {
                    onSnake = true;
                    break;
                }
            }
        }
    }

public:
    SnakeGame() : cellSize(10), offsetX(0), offsetY(20) {} // Offset for status bar

    void init() override {
        randomSeed(millis());
        // Initialize game state
        snake.clear();
        snake.push_back({5, 5});
        snake.push_back({4, 5});
        snake.push_back({3, 5});
        dir = RIGHT;
        nextDir = RIGHT;
        gameOver = false;
        paused = false;
        score = 0;
        lastMoveTime = 0;
        moveInterval = 150; // Speed

        // Calculate grid dimensions based on display
        // Assuming 320x170 (minus status bar)
        gridWidth = 320 / cellSize;
        gridHeight = (170 - offsetY) / cellSize;

        spawnFood();
    }

    void loop() override {
        if (gameOver || paused) return;

        if (millis() - lastMoveTime > moveInterval) {
            lastMoveTime = millis();
            
            // Update direction
            dir = nextDir;

            // Calculate new head position
            Point newHead = snake.front();
            switch (dir) {
                case UP:    newHead.y--; break;
                case DOWN:  newHead.y++; break;
                case LEFT:  newHead.x--; break;
                case RIGHT: newHead.x++; break;
            }

            // Check collisions
            if (newHead.x < 0 || newHead.x >= gridWidth || newHead.y < 0 || newHead.y >= gridHeight) {
                gameOver = true;
                drawGameOver();
                return;
            }

            for (const auto& p : snake) {
                if (p == newHead) {
                    gameOver = true;
                    drawGameOver();
                    return;
                }
            }

            // Move snake
            snake.insert(snake.begin(), newHead);
            Point tailToRemove = {-1, -1};

            // Check food
            if (newHead == food) {
                score++;
                spawnFood();
                if (moveInterval > 50) moveInterval -= 2; // Speed up
            } else {
                tailToRemove = snake.back();
                snake.pop_back();
            }

            // Draw update
            drawUpdate(newHead, tailToRemove);
        }
    }

    void drawFullGame() {
        extern DisplayManager displayManager;
        TFT_eSPI* tft = displayManager.getTFT();
        
        tft->fillRect(0, offsetY, 320, 170 - offsetY, TFT_BLACK);
        
        tft->setTextColor(TFT_GREEN);
        for (const auto& p : snake) {
            tft->fillRect(p.x * cellSize + offsetX, p.y * cellSize + offsetY, cellSize - 1, cellSize - 1, TFT_GREEN);
        }
        tft->fillRect(food.x * cellSize + offsetX, food.y * cellSize + offsetY, cellSize - 1, cellSize - 1, TFT_RED);
        
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->setTextDatum(TL_DATUM);
        tft->drawString("Score: " + String(score), 5, 25, 2);
    }

    void drawUpdate(Point newHead, Point tailToRemove) {
        extern DisplayManager displayManager;
        TFT_eSPI* tft = displayManager.getTFT();

        // Draw new head
        tft->fillRect(newHead.x * cellSize + offsetX, newHead.y * cellSize + offsetY, cellSize - 1, cellSize - 1, TFT_GREEN);

        // Erase tail if needed
        if (tailToRemove.x != -1) {
             tft->fillRect(tailToRemove.x * cellSize + offsetX, tailToRemove.y * cellSize + offsetY, cellSize - 1, cellSize - 1, TFT_BLACK);
        }
        
        // Draw Food (redraw to ensure it's visible if spawned)
        tft->fillRect(food.x * cellSize + offsetX, food.y * cellSize + offsetY, cellSize - 1, cellSize - 1, TFT_RED);

        // Update Score
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->setTextDatum(TL_DATUM);
        tft->drawString("Score: " + String(score), 5, 25, 2);
    }

    void drawGameOver() {
        extern DisplayManager displayManager;
        TFT_eSPI* tft = displayManager.getTFT();
        tft->setTextColor(TFT_RED, TFT_BLACK);
        tft->setTextDatum(MC_DATUM);
        tft->drawString("GAME OVER", 160, 85, 4);
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->drawString("Score: " + String(score), 160, 110, 2);
        tft->drawString("Press Btn 0/1 to Restart", 160, 130, 2);
        tft->drawString("Press Back to Exit", 160, 150, 2);
    }

    String getName() override {
        return "Snake";
    }

    String getDescription() override {
        return "Classic Snake Game";
    }

    void drawMenu(DisplayManager* display) override {
        // Initial draw
        display->clearContent();
        display->drawMenuTitle("Snake");
        drawFullGame();
    }

    bool handleInput(uint8_t button) override {
        if (gameOver) {
            if (button == 2 || button == 3) return false; // Exit on Select or Back
            if (button == 0 || button == 1) { // Restart
                init();
                drawFullGame();
                return true;
            }
        }

        if (button == 3) return false; // Back -> Exit

        if (button == 2) {
            paused = !paused;
            return true;
        }

        // Controls:
        // 0: Up/Left (Turn Left relative to current dir)
        // 1: Down/Right (Turn Right relative to current dir)
        
        if (button == 0) { // Turn Left
            switch (dir) {
                case UP:    nextDir = LEFT; break;
                case DOWN:  nextDir = RIGHT; break;
                case LEFT:  nextDir = DOWN; break;
                case RIGHT: nextDir = UP; break;
            }
        } else if (button == 1) { // Turn Right
            switch (dir) {
                case UP:    nextDir = RIGHT; break;
                case DOWN:  nextDir = LEFT; break;
                case LEFT:  nextDir = UP; break;
                case RIGHT: nextDir = DOWN; break;
            }
        }

        return true;
    }
};
