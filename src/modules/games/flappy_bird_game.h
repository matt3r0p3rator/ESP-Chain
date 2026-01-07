#pragma once
#include <Arduino.h>
#include <vector>
#include "module_base.h"
#include "display_manager.h"
#include "config_manager.h"

#ifndef TFT_SKYBLUE
#define TFT_SKYBLUE 0x867D
#endif
#ifndef TFT_BROWN
#define TFT_BROWN 0x9A60
#endif

class FlappyBirdGame : public Module {
private:
    struct Pipe {
        int x;
        int gapY;
        bool passed;
    };

    struct Rect {
        int x;
        int y;
        int w;
        int h;
    };

    float birdY;
    float birdVelocity;
    const float gravity = 0.6;
    const float lift = -6.0;
    const int birdX = 60;
    const int birdSize = 12;

    std::vector<Pipe> pipes;
    const int pipeWidth = 30;
    const int pipeGap = 60;
    const int pipeSpeed = 3;
    const int pipeInterval = 140; // Pixels between pipes
    
    bool gameOver;
    bool paused;
    int score;
    int highScore;
    unsigned long lastFrameTime;
    const int frameInterval = 30; // ~33 FPS

    int gameWidth = 320;
    int gameHeight = 170;
    int offsetY = 20;
    TFT_eSprite* sprite = nullptr;

    void spawnPipe() {
        int minGapY = offsetY + 20;
        int maxGapY = gameHeight - 20 - pipeGap;
        int gapY = random(minGapY, maxGapY);
        pipes.push_back({gameWidth, gapY, false});
    }

    bool checkCollision(Rect a, Rect b) {
        return (a.x < b.x + b.w && 
                a.x + a.w > b.x && 
                a.y < b.y + b.h && 
                a.y + a.h > b.y);
    }

public:
    FlappyBirdGame() : offsetY(20) {}
    ~FlappyBirdGame() {
        if (sprite) {
            sprite->deleteSprite();
            delete sprite;
            sprite = nullptr;
        }
    }

    void init() override {
        extern DisplayManager displayManager;
        if (!sprite) {
            sprite = new TFT_eSprite(displayManager.getTFT());
            sprite->createSprite(gameWidth, gameHeight - offsetY);
        }

        // Load high score from config
        ConfigManager& config = ConfigManager::getInstance();
        highScore = config.data.flappyBirdHighScore;

        randomSeed(millis());
        birdY = (gameHeight + offsetY) / 2;
        birdVelocity = 0;
        pipes.clear();
        spawnPipe();
        
        gameOver = false;
        paused = false;
        score = 0;
        lastFrameTime = 0;
    }

    void loop() override {
        if (gameOver || paused) return;

        if (millis() - lastFrameTime > frameInterval) {
            lastFrameTime = millis();
            update();
            draw();
        }
    }

    void update() {
        // Update Bird
        birdVelocity += gravity;
        birdY += birdVelocity;

        // Ceiling/Floor collision
        if (birdY < offsetY) {
            birdY = offsetY;
            birdVelocity = 0;
        }
        if (birdY + birdSize > gameHeight) {
            gameOver = true;
        }

        // Update Pipes
        for (int i = 0; i < pipes.size(); i++) {
            pipes[i].x -= pipeSpeed;
        }

        // Remove off-screen pipes
        if (!pipes.empty() && pipes.front().x + pipeWidth < 0) {
            pipes.erase(pipes.begin());
        }

        // Spawn new pipes
        if (pipes.empty() || (gameWidth - pipes.back().x >= pipeInterval)) {
            spawnPipe();
        }

        // Collision Check & Score
        Rect birdRect = {birdX, (int)birdY, birdSize, birdSize};
        
        for (auto& pipe : pipes) {
            // Upper pipe rect
            Rect upperRect = {pipe.x, offsetY, pipeWidth, pipe.gapY - offsetY};
            // Lower pipe rect
            Rect lowerRect = {pipe.x, pipe.gapY + pipeGap, pipeWidth, gameHeight - (pipe.gapY + pipeGap)};

            if (checkCollision(birdRect, upperRect) || checkCollision(birdRect, lowerRect)) {
                gameOver = true;
            }

            if (!pipe.passed && birdX > pipe.x + pipeWidth) {
                score++;
                pipe.passed = true;
            }
        }
    }

    void draw() {
        if (!sprite) return;
        
        // Clear Sprite (Fill Background)
        sprite->fillSprite(TFT_SKYBLUE);

        // Draw Pipes
        sprite->setTextColor(TFT_GREEN); 
        for (const auto& pipe : pipes) {
            // Upper
            sprite->fillRect(pipe.x, 0, pipeWidth, pipe.gapY - offsetY, TFT_GREEN);
            sprite->drawRect(pipe.x, 0, pipeWidth, pipe.gapY - offsetY, TFT_DARKGREEN);
            
            // Lower
            sprite->fillRect(pipe.x, pipe.gapY + pipeGap - offsetY, pipeWidth, gameHeight - (pipe.gapY + pipeGap), TFT_GREEN);
            sprite->drawRect(pipe.x, pipe.gapY + pipeGap - offsetY, pipeWidth, gameHeight - (pipe.gapY + pipeGap), TFT_DARKGREEN);
        }

        // Draw Bird
        sprite->fillRect(birdX, (int)birdY - offsetY, birdSize, birdSize, TFT_YELLOW);
        sprite->drawRect(birdX, (int)birdY - offsetY, birdSize, birdSize, TFT_ORANGE);
        // Eye
        sprite->fillRect(birdX + birdSize - 4, (int)birdY + 2 - offsetY, 2, 2, TFT_BLACK); 

        // Draw Score
        sprite->setTextColor(TFT_WHITE, TFT_SKYBLUE); // bg color to overwrite
        sprite->setTextDatum(TL_DATUM);
        sprite->drawString("Score: " + String(score), 5, 5, 2);
        sprite->drawString("High: " + String(highScore), 210, 5, 2);
        
        // Ground line (at bottom of sprite)
        sprite->drawLine(0, gameHeight - offsetY - 1, gameWidth, gameHeight - offsetY - 1, TFT_BROWN);

        if (gameOver) {
            // Check and save new high score
            if (score > highScore) {
                highScore = score;
                ConfigManager& config = ConfigManager::getInstance();
                config.data.flappyBirdHighScore = highScore;
                config.save();
            }

            sprite->setTextColor(TFT_RED, TFT_BLACK);
            sprite->setTextDatum(MC_DATUM);
            sprite->drawString("GAME OVER", 160, 85 - offsetY, 4);
            sprite->setTextColor(TFT_WHITE, TFT_BLACK);
            sprite->drawString("Score: " + String(score), 160, 110 - offsetY, 2);
            if (score > 0 && score == highScore) {
                sprite->setTextColor(TFT_YELLOW, TFT_BLACK);
                sprite->drawString("NEW HIGH SCORE!", 160, 125 - offsetY, 2);
                sprite->setTextColor(TFT_WHITE, TFT_BLACK);
            } else {
                sprite->drawString("High Score: " + String(highScore), 160, 125 - offsetY, 2);
            }
            sprite->drawString("Press Any Btn Restart", 160, 140 - offsetY, 2);
            sprite->drawString("Press Back to Exit", 160, 155 - offsetY, 2);
        }

        // Push sprite to screen
        sprite->pushSprite(0, offsetY);
    }

    String getName() override {
        return "Flappy Bird";
    }

    String getDescription() override {
        return "Tap to fly!";
    }

    void drawMenu(DisplayManager* display) override {
        display->clearContent();
        display->drawMenuTitle("Flappy Bird");
        draw();
    }

    bool handleInput(uint8_t button) override {
        if (gameOver) {
            if (button == 3) return false; // Exit
             // Restart on any other button
            init();
            return true;
        }

        if (button == 3) return false; // Back -> Exit

        // Flap on any main button
        if (button == 0 || button == 1 || button == 2) {
            birdVelocity = lift;
        }

        return true;
    }
};
