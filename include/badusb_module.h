#pragma once
#include "module_base.h"
#include "sd_manager.h"
#include <vector>

class BadUSBModule : public Module {
public:
    void init() override;
    void loop() override;
    String getName() override;
    const unsigned char* getIcon() override;
    int getIconWidth() override;
    int getIconHeight() override;
    int getIconOffsetY() override;
    int getIconSpacing() override;
    String getDescription() override;
    void drawMenu(DisplayManager* display) override;
    bool handleInput(uint8_t button) override;

private:
    std::vector<FileEntry> scriptFiles;
    int selectedIndex = 0;
    bool filesLoaded = false;
    String statusMessage = "";
    String currentPath = "/payloads";
    
    enum State {
        STATE_BROWSING,
        STATE_ARMED,
        STATE_WAITING_DELAY,
        STATE_RUNNING,
        STATE_DONE
    };
    State state = STATE_BROWSING;
    String selectedPayload = "";
    unsigned long armedTime = 0;
    bool waitingForUSB = false;
};
