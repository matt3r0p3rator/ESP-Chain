#include "wifi_module.h"
#include "WiFi.h"
#include "config_manager.h"
#include "display_manager.h"
#include "../../ui/icons.h"
#include <SD.h>
#include "esp_wifi.h"
#include <FS.h>

extern DisplayManager displayManager;

// Global sniffer stats (needed for callback)
static SnifferStats snifferStats;
static bool snifferActive = false;
static File pcapFile;
static bool pcapFileOpen = false;
static uint32_t pcapPacketCount = 0;

// PCAP file format structures
#pragma pack(push, 1)
struct PcapGlobalHeader {
    uint32_t magic_number;   // 0xa1b2c3d4 for microsecond resolution
    uint16_t version_major;  // 2
    uint16_t version_minor;  // 4
    int32_t  thiszone;       // GMT offset (0)
    uint32_t sigfigs;        // timestamp accuracy (0)
    uint32_t snaplen;        // max packet length (65535)
    uint32_t network;        // link layer type (127 = IEEE 802.11 with radiotap)
};

struct PcapPacketHeader {
    uint32_t ts_sec;         // timestamp seconds
    uint32_t ts_usec;        // timestamp microseconds
    uint32_t incl_len;       // number of bytes saved
    uint32_t orig_len;       // actual packet length
};

// Minimal radiotap header for 802.11 captures
struct RadiotapHeader {
    uint8_t version;         // 0
    uint8_t pad;
    uint16_t length;         // header length in little-endian (8 bytes)
    uint32_t present;        // present flags (0 = no fields)
} __attribute__((packed));
#pragma pack(pop)

// Write packet to PCAP file (called from sniffer callback)
void writePcapPacket(const uint8_t* payload, uint32_t len) {
    if (!pcapFileOpen || !pcapFile) return;
    
    // Validate payload pointer and length
    if (!payload || len == 0 || len > 2048) return;
    
    // Create minimal radiotap header
    RadiotapHeader rtHeader;
    rtHeader.version = 0;
    rtHeader.pad = 0;
    rtHeader.length = 8;  // 8 bytes for minimal header (already in correct little-endian format on ESP32)
    rtHeader.present = 0; // No additional fields
    
    // Calculate total packet length (radiotap + 802.11 frame)
    uint32_t totalLen = sizeof(RadiotapHeader) + len;
    
    // Create packet header with proper timestamps
    PcapPacketHeader pktHeader;
    unsigned long ms = millis();
    pktHeader.ts_sec = ms / 1000;
    pktHeader.ts_usec = (ms % 1000) * 1000;  // Convert ms to microseconds
    pktHeader.incl_len = totalLen;
    pktHeader.orig_len = totalLen;
    
    // Write packet header, radiotap header, then 802.11 frame
    size_t written = 0;
    written += pcapFile.write((uint8_t*)&pktHeader, sizeof(PcapPacketHeader));
    written += pcapFile.write((uint8_t*)&rtHeader, sizeof(RadiotapHeader));
    written += pcapFile.write(payload, len);
    
    // Only increment count if all data was written successfully
    if (written == (sizeof(PcapPacketHeader) + sizeof(RadiotapHeader) + len)) {
        pcapPacketCount++;
        
        // Flush periodically to avoid data loss and ensure file integrity
        if (pcapPacketCount % 10 == 0) {
            pcapFile.flush();
        }
    }
}

// WiFi packet sniffer callback
void IRAM_ATTR wifiSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (!snifferActive) return;
    
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    wifi_pkt_rx_ctrl_t ctrl = pkt->rx_ctrl;
    
    snifferStats.totalPackets++;
    
    if (type == WIFI_PKT_MGMT) {
        // Management frame - check subtype in frame control
        uint8_t frameControl = pkt->payload[0];
        uint8_t subtype = (frameControl >> 4) & 0x0F;
        
        switch (subtype) {
            case 0x08: // Beacon
                snifferStats.beacons++;
                break;
            case 0x04: // Probe Request
                snifferStats.probeReq++;
                break;
            case 0x05: // Probe Response
                snifferStats.probeResp++;
                break;
            case 0x0B: // Authentication
                snifferStats.auth++;
                break;
            case 0x0C: // Deauthentication
                snifferStats.deauth++;
                break;
            case 0x00: // Association Request
            case 0x01: // Association Response
                snifferStats.assoc++;
                break;
            case 0x0A: // Disassociation
                snifferStats.disassoc++;
                break;
            default:
                snifferStats.other++;
                break;
        }
    } else if (type == WIFI_PKT_DATA) {
        snifferStats.data++;
    } else {
        snifferStats.other++;
    }
    
    // Write packet to PCAP file - use actual payload length
    if (pcapFileOpen && pkt->rx_ctrl.sig_len > 0 && pkt->rx_ctrl.sig_len <= 2048) {
        // Validate reasonable packet size to prevent corruption
        writePcapPacket(pkt->payload, pkt->rx_ctrl.sig_len);
    }
}

String WiFiModule::getEncryptionType(wifi_auth_mode_t encryption) {
    switch (encryption) {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-E";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/3";
        case WIFI_AUTH_WAPI_PSK: return "WAPI";
        default: return "UNKN";
    }
}

String WiFiModule::getSignalStrength(int32_t rssi) {
    if (rssi >= -50) return "Excellent";
    if (rssi >= -60) return "Good";
    if (rssi >= -70) return "Fair";
    if (rssi >= -80) return "Weak";
    return "Very Weak";
}

void WiFiModule::init() {
    currentState = MENU;
    menuIndex = 0;
    selectedIndex = -1;
    selectedSSID = "";
    selectedBSSID = "";
    scrollOffset = 0;
    actionMenuIndex = 0;
    isScanning = false;
    isSniffing = false;
    snifferChannel = 1;
    lastSnifferUpdate = 0;
    lastScanUpdate = 0;
    lastNetworkCount = 0;
    
    // Initialize sniffer display cache
    lastDisplayedTotal = 0;
    lastDisplayedBeacons = 0;
    lastDisplayedDeauth = 0;
    lastDisplayedData = 0;
    lastDisplayedRuntime = 0;
    lastDisplayedPcapStatus = false;
}

void WiFiModule::loop() {
    if (isScanning) {
        // Check if scan is complete
        int n = WiFi.scanComplete();
        if (n >= 0) {
            Serial.printf("DEBUG: Scan complete, found %d networks\n", n);
            
            // Scan finished - rebuild network list
            std::vector<NetworkInfo> newNetworks;
            
            for (int i = 0; i < n; i++) {
                NetworkInfo info;
                info.ssid = WiFi.SSID(i);
                info.bssid = WiFi.BSSIDstr(i); // Get MAC address
                info.rssi = WiFi.RSSI(i);
                info.encryption = WiFi.encryptionType(i);
                info.channel = WiFi.channel(i);
                newNetworks.push_back(info);
                Serial.printf("DEBUG: Network %d: %s (BSSID: %s, RSSI: %d)\n", 
                              i, info.ssid.c_str(), info.bssid.c_str(), info.rssi);
            }
            
            Serial.printf("DEBUG: newNetworks.size() = %d\n", newNetworks.size());
            
            // Only update if we're viewing scan results and list changed
            bool shouldUpdate = false;
            if (currentState == VIEW_SCAN && (networkCount != n || networks.size() != newNetworks.size())) {
                shouldUpdate = true;
                // Validate selectedIndex will still be valid after update
                if (selectedIndex >= newNetworks.size()) {
                    selectedIndex = max(0, (int)newNetworks.size() - 1);
                }
            }
            
            // Update the network list atomically to prevent flashing
            networks = std::move(newNetworks);
            networkCount = n;
            lastNetworkCount = networkCount;
            
            Serial.println("====================================");
            Serial.printf("Total Networks Found: %d\n", networkCount);
            Serial.println("====================================");
            for (int i = 0; i < networks.size(); i++) {
                Serial.printf("[%d] SSID: %s\n", i, networks[i].ssid.c_str());
                Serial.printf("    BSSID: %s\n", networks[i].bssid.c_str());
                Serial.printf("    RSSI: %d dBm\n", networks[i].rssi);
                Serial.printf("    Channel: %d\n", networks[i].channel);
                Serial.printf("    Encryption: %s\n", getEncryptionType(networks[i].encryption).c_str());
                Serial.println("------------------------------------");
            }
            Serial.println("====================================");
            Serial.printf("networks.size()=%d, networkCount=%d\n", 
                          networks.size(), networkCount);
            
            // Update display only if needed and with a small debounce
            if (shouldUpdate && (millis() - lastScanUpdate >= 500)) {
                lastScanUpdate = millis();
                drawMenu(displayManager);
            }
            
            // Start next scan
            WiFi.scanDelete();
            WiFi.scanNetworks(true); // Async scan
        } else if (n == WIFI_SCAN_FAILED) {
            // Scan failed, restart
            WiFi.scanDelete();
            WiFi.scanNetworks(true);
        }
    }
    
}

void WiFiModule::drawMenu(DisplayManager* display) {
    if (!display || !display->getTFT()) return;
    
    this->displayManager = display;

    if (currentState == MENU) {
        display->clearMenu();
        display->drawMenuItem("Toggle Scan", 0, menuIndex == 0);
        display->drawMenuItem("View Scan Results", 1, menuIndex == 1);
        display->updateMenu();
        if (isScanning) {
            display->getTFT()->setTextDatum(MC_DATUM);
            display->getTFT()->setTextColor(TFT_GREEN, TFT_BLACK);
            display->getTFT()->setTextSize(1);
            display->getTFT()->drawString("Scanning", display->getTFT()->width() / 2, display->getTFT()->height() - 10, 2); // Bottom middle
        }
    }
    else if (currentState == VIEW_SCAN) {
        display->clearMenu();
        for (int i = 0; i < networks.size(); i++) {
            String ssid = networks[i].ssid;
            if (ssid.length() > 25) ssid = ssid.substring(0, 25);
            String signal = getSignalStrength(networks[i].rssi);
            String itemText = ssid + " [" + signal + "]";
            display->drawMenuItem(itemText, i, i == selectedIndex);
        }
        display->updateMenu();
        if (isScanning) {
            display->getTFT()->setTextDatum(MC_DATUM);
            display->getTFT()->setTextColor(TFT_GREEN, TFT_BLACK);
            display->getTFT()->setTextSize(1);
            display->getTFT()->drawString("Scanning", display->getTFT()->width() / 2, display->getTFT()->height() - 10, 2); // Bottom middle
        } else {
            display->getTFT()->setTextDatum(MC_DATUM);
            display->getTFT()->setTextColor(TFT_RED, TFT_BLACK);
            display->getTFT()->setTextSize(1);
            display->getTFT()->drawString("Scan Stopped", display->getTFT()->width() / 2, display->getTFT()->height() / 2 + 20, 2); // Bottom middle
        }
    }
    else if (currentState == VIEW_DETAILS) {
        display->clearMenu();
        
        int netIndex = getSelectedNetworkIndex();
        if (netIndex >= 0 && netIndex < networks.size()) {
            NetworkInfo& net = networks[netIndex];
            
            display->drawMenuTitle("Network Details");
            
            // Show details as menu items (non-selectable)
            String ssid = "SSID: " + net.ssid;
            if (ssid.length() > 25) ssid = ssid.substring(0, 25);
            display->drawMenuItem(ssid, 0, false);
            
            String signal = "Signal: " + String(net.rssi) + "dBm";
            display->drawMenuItem(signal, 1, false);
            
            String strength = "Quality: " + getSignalStrength(net.rssi);
            display->drawMenuItem(strength, 2, false);
            
            String security = "Security: " + getEncryptionType(net.encryption);
            display->drawMenuItem(security, 3, false);
            
            String channel = "Channel: " + String(net.channel);
            display->drawMenuItem(channel, 4, false);
            
            display->getTFT()->setTextDatum(TL_DATUM);
            display->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
            display->getTFT()->drawString("BACK: Return", 10, 220, 2);
        }
        
        display->updateMenu();
    }
    else if (currentState == NETWORK_ACTIONS) {
        display->clearMenu();
        
        Serial.println("DEBUG: Drawing NETWORK_ACTIONS menu");
        
        int netIndex = getSelectedNetworkIndex();
        Serial.printf("DEBUG: getSelectedNetworkIndex returned %d\n", netIndex);
        
        if (netIndex >= 0 && netIndex < networks.size()) {
            String title = networks[netIndex].ssid;
            if (title.length() > 20) title = title.substring(0, 20);
            display->drawMenuTitle(title.c_str());
            
            display->drawMenuItem("View Details", 0, actionMenuIndex == 0);
            display->drawMenuItem("Sniff Channel", 1, actionMenuIndex == 1);
            display->drawMenuItem("Save to SD Card", 2, actionMenuIndex == 2);
            display->drawMenuItem("Copy SSID", 3, actionMenuIndex == 3);
            display->drawMenuItem("Back", 4, actionMenuIndex == 4);
        } else {
            Serial.println("DEBUG: Network index invalid, showing error");
            display->getTFT()->setTextDatum(TL_DATUM);
            display->getTFT()->setTextColor(TFT_RED, TFT_BLACK);
            display->getTFT()->drawString("Network not found!", 10, 40, 2);
            display->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
            display->getTFT()->drawString("BACK: Return", 10, 220, 2);
        }
        
        display->updateMenu();
    }
    else if (currentState == SNIFFING) {
        drawSnifferUI();
    }
}

bool WiFiModule::handleInput(uint8_t button) {
    // 0=Up, 1=Down, 2=Select, 3=Back

    if (!displayManager) return true;

    if (currentState == MENU) {
        if (button == 0) { // Up
            menuIndex--;
            if (menuIndex < 0) menuIndex = 1;
            drawMenu(displayManager); // Force redraw
        } else if (button == 1) { // Down
            menuIndex++;
            if (menuIndex > 1) menuIndex = 0;
            drawMenu(displayManager); // Force redraw
        } else if (button == 2) { // Select
            if (menuIndex == 0) {
                // Start/Stop Scan
                if (!isScanning) {
                    // Start Scan
                    Serial.println("DEBUG: Starting WiFi scan");
                    
                    // Ensure WiFi is fully stopped first
                    esp_wifi_set_promiscuous(false);
                    WiFi.mode(WIFI_OFF);
                    delay(100);
                    
                    // Initialize WiFi in station mode
                    WiFi.mode(WIFI_STA);
                    WiFi.disconnect();
                    delay(100);
                    
                    networks.clear();
                    networkCount = 0;
                    
                    // Start scan with explicit parameters (all channels, show hidden)
                    WiFi.scanNetworks(true, true); // async=true, show_hidden=true
                    isScanning = true;
                    
                    Serial.println("DEBUG: Scan started");
                } else {
                    // Stop Scan
                    WiFi.scanDelete();
                    isScanning = false;
                }
                drawMenu(displayManager); // Force redraw
            } else if (menuIndex == 1) {
                currentState = VIEW_SCAN;
                scrollOffset = 0;
                // Initialize selectedIndex properly
                if (networks.empty()) {
                    selectedIndex = -1;
                } else {
                    selectedIndex = 0;
                }
                // Keep scanning active while viewing results for live updates
                drawMenu(displayManager); // Force redraw
            }
        } else if (button == 3) { // Back
            // Stop any scanning when exiting module
            if (isScanning) {
                WiFi.scanDelete();
                isScanning = false;
            }
            return false; // Exit module
        }
    }
    else if (currentState == VIEW_SCAN) {
        int itemsPerPage = 5;
        
        if (button == 0) { // Up
            if (networks.size() == 0) return true;
            
            selectedIndex--;
            if (selectedIndex < 0) {
                selectedIndex = networks.size() - 1;
                scrollOffset = max(0, (int)networks.size() - itemsPerPage);
            } else if (selectedIndex < scrollOffset) {
                scrollOffset--;
            }
            
            Serial.printf("DEBUG: UP - selectedIndex=%d, scrollOffset=%d\n", selectedIndex, scrollOffset);
            drawMenu(displayManager);
            
        } else if (button == 1) { // Down
            if (networks.size() == 0) return true;
            
            selectedIndex++;
            if (selectedIndex >= networks.size()) {
                selectedIndex = 0;
                scrollOffset = 0;
            } else if (selectedIndex >= scrollOffset + itemsPerPage) {
                scrollOffset++;
            }
            
            Serial.printf("DEBUG: DOWN - selectedIndex=%d, scrollOffset=%d\n", selectedIndex, scrollOffset);
            drawMenu(displayManager);
        } else if (button == 2) { // Select
            Serial.printf("DEBUG: SELECT pressed, selectedIndex=%d, networks.size()=%d\n", selectedIndex, networks.size());
            
            // Validate we have networks
            if (networks.empty()) {
                Serial.println("DEBUG: No networks available");
                return true;
            }
            
            // Ensure selectedIndex is valid
            if (selectedIndex < 0) {
                selectedIndex = 0;
                Serial.println("DEBUG: selectedIndex was negative, reset to 0");
            }
            if (selectedIndex >= networks.size()) {
                selectedIndex = networks.size() - 1;
                Serial.printf("DEBUG: selectedIndex was too high, reset to %d\n", selectedIndex);
            }
            
            Serial.printf("DEBUG: Selecting network at index %d\n", selectedIndex);
            
            // Store both SSID and BSSID for reliable identification
            selectedSSID = networks[selectedIndex].ssid;
            selectedBSSID = networks[selectedIndex].bssid;
            
            Serial.printf("DEBUG: Selected SSID: '%s'\n", selectedSSID.c_str());
            Serial.printf("DEBUG: Selected BSSID: '%s'\n", selectedBSSID.c_str());
            
            // Transition to network actions
            currentState = NETWORK_ACTIONS;
            actionMenuIndex = 0;
            Serial.println("DEBUG: State changed to NETWORK_ACTIONS");
            
            // Force redraw
            drawMenu(displayManager);
            Serial.println("DEBUG: Menu redrawn");
        } else if (button == 3) { // Back
            Serial.println("DEBUG: BACK pressed from VIEW_SCAN");
            currentState = MENU;
            scrollOffset = 0;
            // Keep scanning active when returning to menu
            drawMenu(displayManager);
        }
    }
    else if (currentState == NETWORK_ACTIONS) {
        if (button == 0) { // Up
            actionMenuIndex--;
            if (actionMenuIndex < 0) actionMenuIndex = 4;
            drawMenu(displayManager);
        } else if (button == 1) { // Down
            actionMenuIndex++;
            if (actionMenuIndex > 4) actionMenuIndex = 0;
            drawMenu(displayManager);
        } else if (button == 2) { // Select
            if (actionMenuIndex == 0) {
                // View Details - show detailed network info
                currentState = VIEW_DETAILS;
                drawMenu(displayManager);
            } else if (actionMenuIndex == 1) {
                // Sniff Channel - start packet sniffer on network's channel
                int netIndex = getSelectedNetworkIndex();
                if (netIndex >= 0 && netIndex < networks.size()) {
                    snifferChannel = networks[netIndex].channel;
                    startSniffer(snifferChannel);
                    currentState = SNIFFING;
                    drawMenu(displayManager);
                }
            } else if (actionMenuIndex == 2) {
                // Save to SD Card
                saveNetworkToSD();
            } else if (actionMenuIndex == 3) {
                // Copy SSID (store in a variable for later use)
                displayManager->getTFT()->fillScreen(TFT_BLACK);
                displayManager->getTFT()->setTextDatum(MC_DATUM);
                displayManager->getTFT()->setTextColor(TFT_GREEN, TFT_BLACK);
                displayManager->getTFT()->drawString("SSID Copied!", 120, 135, 4);
                delay(1000);
                currentState = VIEW_SCAN;
                drawMenu(displayManager);
            } else if (actionMenuIndex == 4) {
                // Back to scan results
                currentState = VIEW_SCAN;
                drawMenu(displayManager);
            }
        } else if (button == 3) { // Back
            currentState = VIEW_SCAN;
            drawMenu(displayManager);
        }
    }
    else if (currentState == VIEW_DETAILS) {
        // Any button returns to actions menu
        currentState = NETWORK_ACTIONS;
        drawMenu(displayManager);
    }
    else if (currentState == SNIFFING) {
        if (button == 3) { // Back - stop sniffer and return
            stopSniffer();
            
            // Show save confirmation
            displayManager->getTFT()->fillScreen(TFT_BLACK);
            displayManager->getTFT()->setTextDatum(MC_DATUM);
            
            if (pcapPacketCount > 0) {
                displayManager->getTFT()->setTextColor(TFT_GREEN, TFT_BLACK);
                displayManager->getTFT()->drawString("PCAP Saved!", 120, 100, 4);
                displayManager->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
                displayManager->getTFT()->drawString(String(pcapPacketCount) + " packets captured", 120, 140, 2);
            } else {
                displayManager->getTFT()->setTextColor(TFT_YELLOW, TFT_BLACK);
                displayManager->getTFT()->drawString("Sniffer Stopped", 120, 100, 4);
                displayManager->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
                displayManager->getTFT()->drawString("No SD or no packets", 120, 140, 2);
            }
            delay(1500);
            
            currentState = NETWORK_ACTIONS;
            drawMenu(displayManager);
        }
        // Other buttons do nothing while sniffing
    }

    return true;
}

int WiFiModule::getSelectedNetworkIndex() {
    // First try to match by BSSID (most reliable)
    if (selectedBSSID.length() > 0) {
        for (int i = 0; i < networks.size(); i++) {
            if (networks[i].bssid == selectedBSSID) {
                return i;
            }
        }
    }
    
    // Fallback to SSID match (less reliable due to duplicate SSIDs)
    if (selectedSSID.length() > 0) {
        for (int i = 0; i < networks.size(); i++) {
            if (networks[i].ssid == selectedSSID) {
                return i;
            }
        }
    }
    
    return -1; // Not found
}

void WiFiModule::showNetworkDetails() {
    // Details are now shown in drawMenu
}

void WiFiModule::saveNetworkToSD() {
    int netIndex = getSelectedNetworkIndex();
    if (netIndex < 0 || netIndex >= networks.size()) return;
    
    NetworkInfo& net = networks[netIndex];
    
    displayManager->getTFT()->fillScreen(TFT_BLACK);
    displayManager->getTFT()->setTextDatum(MC_DATUM);
    
    // Check if SD is available
    if (!SD.begin()) {
        displayManager->getTFT()->setTextColor(TFT_RED, TFT_BLACK);
        displayManager->getTFT()->drawString("SD Card Error!", 120, 100, 4);
        displayManager->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
        displayManager->getTFT()->drawString("Insert SD card", 120, 140, 2);
        delay(2000);
        currentState = NETWORK_ACTIONS;
        drawMenu(displayManager);
        return;
    }
    
    // Create or append to wifi_networks.txt
    File file = SD.open("/wifi_networks.txt", FILE_APPEND);
    if (!file) {
        displayManager->getTFT()->setTextColor(TFT_RED, TFT_BLACK);
        displayManager->getTFT()->drawString("File Error!", 120, 120, 4);
        delay(2000);
        currentState = NETWORK_ACTIONS;
        drawMenu(displayManager);
        return;
    }
    
    // Write network info
    file.println("=====================================");
    file.println("SSID: " + net.ssid);
    file.println("RSSI: " + String(net.rssi) + " dBm (" + getSignalStrength(net.rssi) + ")");
    file.println("Encryption: " + getEncryptionType(net.encryption));
    file.println("Channel: " + String(net.channel));
    file.println("Timestamp: " + String(millis() / 1000) + "s");
    file.println("=====================================");
    file.println();
    file.close();
    
    // Show success message
    displayManager->getTFT()->setTextColor(TFT_GREEN, TFT_BLACK);
    displayManager->getTFT()->drawString("Saved!", 120, 100, 4);
    displayManager->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
    displayManager->getTFT()->drawString("wifi_networks.txt", 120, 140, 2);
    delay(1500);
    
    currentState = NETWORK_ACTIONS;
    drawMenu(displayManager);
}

void WiFiModule::startSniffer(int channel) {
    // Stop any existing WiFi operations
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    
    // Reset stats
    snifferStats.totalPackets = 0;
    snifferStats.beacons = 0;
    snifferStats.probeReq = 0;
    snifferStats.probeResp = 0;
    snifferStats.auth = 0;
    snifferStats.deauth = 0;
    snifferStats.assoc = 0;
    snifferStats.disassoc = 0;
    snifferStats.data = 0;
    snifferStats.other = 0;
    snifferStats.startTime = millis();
    pcapPacketCount = 0;
    lastSnifferUpdate = 0;
    
    // Initialize PCAP file
    initPcapFile();
    
    // Set channel
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    
    // Enable promiscuous mode
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&wifiSnifferCallback);
    
    snifferActive = true;
    isSniffing = true;
    snifferChannel = channel;
}

void WiFiModule::stopSniffer() {
    snifferActive = false;
    isSniffing = false;
    
    // Disable promiscuous mode
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(nullptr);
    
    // Close PCAP file
    closePcapFile();
    
    // Reset WiFi
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
}

void WiFiModule::initPcapFile() {
    // Initialize SD card
    if (!SD.begin()) {
        pcapFileOpen = false;
        return;
    }
    
    // Create captures directory if it doesn't exist
    if (!SD.exists("/captures")) {
        SD.mkdir("/captures");
    }
    
    // Get selected network SSID and sanitize it for filename
    String networkName = "unknown";
    int netIndex = getSelectedNetworkIndex();
    if (netIndex >= 0 && netIndex < networks.size()) {
        networkName = networks[netIndex].ssid;
        // Replace invalid filename characters
        networkName.replace("/", "_");
        networkName.replace("\\", "_");
        networkName.replace(":", "_");
        networkName.replace("*", "_");
        networkName.replace("?", "_");
        networkName.replace("\"", "_");
        networkName.replace("<", "_");
        networkName.replace(">", "_");
        networkName.replace("|", "_");
        networkName.replace(" ", "_");
        // Limit length to avoid filesystem issues
        if (networkName.length() > 20) {
            networkName = networkName.substring(0, 20);
        }
        if (networkName.length() == 0) {
            networkName = "unknown";
        }
    }
    
    // Create filename with network name and timestamp
    String filename = "/captures/" + networkName + "_" + String(millis()) + ".pcap";
    
    pcapFile = SD.open(filename, FILE_WRITE);
    if (!pcapFile) {
        pcapFileOpen = false;
        return;
    }
    
    // Write PCAP global header
    PcapGlobalHeader header;
    header.magic_number = 0xa1b2c3d4;  // Microsecond resolution
    header.version_major = 2;
    header.version_minor = 4;
    header.thiszone = 0;
    header.sigfigs = 0;
    header.snaplen = 65535;
    header.network = 127;  // IEEE 802.11 with radiotap header
    
    pcapFile.write((uint8_t*)&header, sizeof(PcapGlobalHeader));
    pcapFile.flush();
    
    pcapFileOpen = true;
    pcapPacketCount = 0;
}

void WiFiModule::closePcapFile() {
    if (pcapFileOpen && pcapFile) {
        pcapFile.flush();
        pcapFile.close();
        pcapFileOpen = false;
    }
}

void WiFiModule::drawSnifferUI() {
    if (!displayManager || !displayManager->getTFT()) return;
    
    TFT_eSPI* tft = displayManager->getTFT();
    
    unsigned long runtime = (millis() - snifferStats.startTime) / 1000;
    bool pcapStatus = pcapFileOpen;
    
    // Check if key values have changed to minimize flashing
    bool needsRedraw = (snifferStats.totalPackets != lastDisplayedTotal ||
                       snifferStats.beacons != lastDisplayedBeacons ||
                       snifferStats.deauth != lastDisplayedDeauth ||
                       snifferStats.data != lastDisplayedData ||
                       runtime != lastDisplayedRuntime ||
                       pcapStatus != lastDisplayedPcapStatus);
    
    if (!needsRedraw) return;
    
    // Clear entire screen including top bar
    tft->fillScreen(TFT_BLACK);
    
    // Title
    tft->setTextDatum(TC_DATUM);
    tft->setTextColor(TFT_CYAN, TFT_BLACK);
    tft->drawString("Packet Sniffer", 120, 5, 2);
    
    // Channel and runtime with recording indicator
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    String statusLine = "Ch:" + String(snifferChannel) + " | " + String(runtime) + "s";
    if (pcapFileOpen) {
        statusLine += " | REC";
    } else {
        statusLine += " | No SD";
    }
    tft->drawString(statusLine, 120, 25, 2);
    
    // Stats - left column
    tft->setTextDatum(TL_DATUM);
    int y = 50;
    int lineHeight = 18;
    
    tft->setTextColor(TFT_GREEN, TFT_BLACK);
    String totalText = "Total: " + String(snifferStats.totalPackets);
    if (pcapFileOpen) {
        totalText += " (PCAP)";
    }
    tft->drawString(totalText, 10, y, 2);
    y += lineHeight;
    
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString("Beacons: " + String(snifferStats.beacons), 10, y, 2);
    y += lineHeight;
    
    tft->drawString("ProbeReq: " + String(snifferStats.probeReq), 10, y, 2);
    y += lineHeight;
    
    tft->drawString("ProbeRsp: " + String(snifferStats.probeResp), 10, y, 2);
    y += lineHeight;
    
    tft->drawString("Auth: " + String(snifferStats.auth), 10, y, 2);
    
    // Stats - right column
    y = 50 + lineHeight;
    tft->setTextColor(TFT_RED, TFT_BLACK);
    tft->drawString("Deauth: " + String(snifferStats.deauth), 130, y, 2);
    y += lineHeight;
    
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString("Assoc: " + String(snifferStats.assoc), 130, y, 2);
    y += lineHeight;
    
    tft->drawString("Disassoc: " + String(snifferStats.disassoc), 130, y, 2);
    y += lineHeight;
    
    tft->setTextColor(TFT_BLUE, TFT_BLACK);
    tft->drawString("Data: " + String(snifferStats.data), 130, y, 2);
    y += lineHeight;
    
    tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft->drawString("Other: " + String(snifferStats.other), 130, y, 2);
    
    // Instructions at bottom
    tft->setTextDatum(BC_DATUM);
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    tft->drawString("BACK: Stop & Save", 120, 165, 2);
    
    // Update cache values
    lastDisplayedTotal = snifferStats.totalPackets;
    lastDisplayedBeacons = snifferStats.beacons;
    lastDisplayedDeauth = snifferStats.deauth;
    lastDisplayedData = snifferStats.data;
    lastDisplayedRuntime = runtime;
    lastDisplayedPcapStatus = pcapStatus;
}

void WiFiModule::saveSnifferLogToSD() {
    if (!displayManager) return;
    
    displayManager->getTFT()->fillScreen(TFT_BLACK);
    displayManager->getTFT()->setTextDatum(MC_DATUM);
    
    // Check if SD is available
    if (!SD.begin()) {
        displayManager->getTFT()->setTextColor(TFT_RED, TFT_BLACK);
        displayManager->getTFT()->drawString("SD Card Error!", 120, 100, 4);
        displayManager->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
        displayManager->getTFT()->drawString("Insert SD card", 120, 140, 2);
        delay(2000);
        drawSnifferUI();
        return;
    }
    
    // Create or append to sniffer_log.txt
    File file = SD.open("/sniffer_log.txt", FILE_APPEND);
    if (!file) {
        displayManager->getTFT()->setTextColor(TFT_RED, TFT_BLACK);
        displayManager->getTFT()->drawString("File Error!", 120, 120, 4);
        delay(2000);
        drawSnifferUI();
        return;
    }
    
    // Write sniffer stats
    unsigned long runtime = (millis() - snifferStats.startTime) / 1000;
    file.println("=====================================");
    file.println("SNIFFER LOG - Channel: " + String(snifferChannel));
    file.println("Runtime: " + String(runtime) + " seconds");
    file.println("-------------------------------------");
    file.println("Total Packets: " + String(snifferStats.totalPackets));
    file.println("Beacons: " + String(snifferStats.beacons));
    file.println("Probe Requests: " + String(snifferStats.probeReq));
    file.println("Probe Responses: " + String(snifferStats.probeResp));
    file.println("Auth: " + String(snifferStats.auth));
    file.println("Deauth: " + String(snifferStats.deauth));
    file.println("Assoc: " + String(snifferStats.assoc));
    file.println("Disassoc: " + String(snifferStats.disassoc));
    file.println("Data: " + String(snifferStats.data));
    file.println("Other: " + String(snifferStats.other));
    file.println("=====================================");
    file.println();
    file.close();
    
    // Show success message
    displayManager->getTFT()->setTextColor(TFT_GREEN, TFT_BLACK);
    displayManager->getTFT()->drawString("Saved!", 120, 100, 4);
    displayManager->getTFT()->setTextColor(TFT_WHITE, TFT_BLACK);
    displayManager->getTFT()->drawString("sniffer_log.txt", 120, 140, 2);
    delay(1500);
    
    drawSnifferUI();
}
