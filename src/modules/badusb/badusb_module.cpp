#include "badusb_module.h"

void BadUSBModule::init() {
    // Init USB HID here
}

void BadUSBModule::loop() {
    // Attack loop
}

String BadUSBModule::getName() {
    return "BadUSB";
}

String BadUSBModule::getDescription() {
    return "HID Attacks";
}

void BadUSBModule::drawMenu(TFT_eSPI &tft) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("BadUSB Module", 10, 10);
    tft.drawString("Press Back to Exit", 10, 50);
}

void BadUSBModule::handleInput(uint8_t button) {
    // Handle input
}
