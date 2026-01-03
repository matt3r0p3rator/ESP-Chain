#pragma once
#include <Arduino.h>
#include <vector>
#include <WiFi.h>
#include "module_base.h"
#include "display_manager.h"
#include "../../ui/icons.h"

struct APInfo {
    String ssid;
    int32_t rssi;
    uint8_t channel;
    String bssid;
    wifi_auth_mode_t encryption;
    unsigned long lastSeen;
};

class WiFiModule : public Module {
public:
    // Module Interface
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

    // Attack / Sniffer Methods (Used by wifi_handshake_cap.cpp)
    static void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    void startDeauth();
    void stopDeauth();
    void startHandshakeCapture();
    void stopHandshakeCapture();
    void startMixedAttack();
    void stopMixedAttack();
    void startStationScan();
    void stopStationScan();
    
    // Helper for attack UI updates
    void updateUI(DisplayManager* display);

private:
    // State Management
    enum State {
        MENU,
        SCANNER_MENU,
        RESULTS,
        DETAILS,
        TARGET_OPTIONS,
        ATTACK_DEAUTH,
        HANDSHAKE_CAPTURE,
        ATTACK_MIXED,
        STATION_SCAN,
        STATION_LIST,
        SETTINGS,
        SETTINGS_SCAN_TIME,
        SETTINGS_SHOW_HIDDEN,
        SETTINGS_SORT_METHOD
    };

    enum SortMethod {
        SORT_RSSI,
        SORT_CHANNEL
    };

    State currentState;
    
    // Navigation
    int menuIndex;
    int selectedIndex;   // Index in the scanResults vector
    int settingsIndex;
    
    // Scanner Data
    std::vector<APInfo> scanResults;
    bool isScanning;
    unsigned long lastScanTime;
    
    // Selection
    APInfo selectedTarget;

    // Attack Data (Accessed by wifi_handshake_cap.cpp)
    bool isDeauthing = false;
    bool isCapturing = false;
    bool isMixedAttack = false;
    bool isScanningStations = false;
    int deauthPacketsSent = 0;
    int handshakesCaptured = 0;
    
    // Station scanning
    std::vector<String> detectedStations;
    String selectedStation = ""; // Empty means broadcast/all
    int stationListIndex = 0;

    // Settings
    uint32_t scanTimePerChannel = 300;
    bool showHidden = true;
    SortMethod sortMethod = SORT_RSSI;

    // Menu Items
    const char* menuItems[2] = {"Scan Networks", "Settings"};
    const char* settingsItems[3] = {"Scan Time", "Show Hidden", "Sort By"};

    // Internal Helpers
    void processScanResults(int networksFound);
    void sortResults();
    String getEncryptionName(wifi_auth_mode_t encryption);
    
    // Attack Helpers (implemented in wifi_handshake_cap.cpp)
    void sendDeauthFrame();
    void drawTerminal(DisplayManager* display);
    void drawTerminalUpdate(DisplayManager* display);
    
    // PCAP
    String pcapFileName;
    void openPcapFile();
    void savePacketToSD(uint8_t* buf, int len);

    // Packet Queue
    struct CapturedPacket {
        uint8_t data[512];
        int len;
    };
    QueueHandle_t packetQueue;
    void processPacketQueue();
};
