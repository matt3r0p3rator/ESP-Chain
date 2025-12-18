#pragma once
#include "module_base.h"

class BadUSBModule : public Module {
public:
    void init() override;
    void loop() override;
    String getName() override;
    String getDescription() override;
    void drawMenu(TFT_eSPI &tft) override;
    void handleInput(uint8_t button) override;
};
