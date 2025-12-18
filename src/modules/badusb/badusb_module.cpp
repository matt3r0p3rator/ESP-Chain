#include "module_base.h"

class BadUSBModule : public Module {
public:
    void init() override {}
    void loop() override {}
    String getName() override { return "BadUSB"; }
    String getDescription() override { return "HID Attacks"; }
    void drawMenu(TFT_eSPI &tft) override {}
    void handleInput(uint8_t button) override {}
};
