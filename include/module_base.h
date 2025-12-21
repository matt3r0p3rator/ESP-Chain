#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

class DisplayManager; // Forward declaration

class Module {
public:
  virtual void init() = 0;                          // Initialize hardware
  virtual void loop() = 0;                          // Main loop
  virtual String getName() = 0;                     // Module name
  virtual String getDescription() = 0;              // Short description
  virtual void drawMenu(DisplayManager* display) = 0; // Pass DisplayManager
  virtual bool handleInput(uint8_t button) = 0;    // Handle user input. Return true if handled, false if module should exit.
  virtual ~Module() {}
};
