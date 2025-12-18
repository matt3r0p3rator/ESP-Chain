#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

class Module {
public:
  virtual void init() = 0;                          // Initialize hardware
  virtual void loop() = 0;                          // Main loop
  virtual String getName() = 0;                     // Module name
  virtual String getDescription() = 0;              // Short description
  virtual void drawMenu(TFT_eSPI &tft) = 0;         // Draw module UI
  virtual void handleInput(uint8_t button) = 0;    // Handle user input
  virtual ~Module() {}
};
