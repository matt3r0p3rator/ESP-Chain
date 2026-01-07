#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEAdvertisedDevice.h>
#include <vector>

// Jammer modes
enum JammerMode {
    JAM_CONTINUOUS,      // Continuous interference packets
    JAM_REACTIVE,        // React to target's transmissions
    JAM_DEAUTH           // Send disconnect/deauth-style packets
};

// Target device info structure - simplified to avoid static init issues
struct BLETargetDevice {
    String name;
    String address;
    int rssi;
    bool isConnectable;
    String serviceUUID;  // Store as string instead of BLEUUID vector
    
    BLETargetDevice() : rssi(0), isConnectable(false) {}
};

class BLEJammerUtils {
public:
    // Initialize the jammer
    static void init();
    
    // Set the target device to jam
    static void setTarget(const BLETargetDevice& target);
    
    // Clear the current target
    static void clearTarget();
    
    // Check if a target is set
    static bool hasTarget();
    
    // Get current target info
    static const BLETargetDevice& getTarget();
    
    // Get target name safely
    static String getTargetName();
    
    // Get target RSSI safely
    static int getTargetRSSI();
    
    // Start jamming the target
    static void startJamming(JammerMode mode = JAM_CONTINUOUS);
    
    // Stop jamming
    static void stopJamming();
    
    // Check if currently jamming
    static bool isJamming();
    
    // Get current jammer mode
    static JammerMode getMode();
    
    // Set jammer mode
    static void setMode(JammerMode mode);
    
    // Main loop function - call this frequently when jamming
    static void loop();
    
    // Generate interference advertisement data
    static std::vector<uint8_t> generateInterferenceData();
    
    // Generate targeted advertisement that mimics the target
    static std::vector<uint8_t> generateMimicData(const BLETargetDevice& target);
    
    // Get jammer statistics
    static uint32_t getPacketCount();
    static void resetPacketCount();
    
    // Utility to parse a scanned device into BLETargetDevice
    static BLETargetDevice parseDevice(BLEAdvertisedDevice& device);

private:
    static BLETargetDevice currentTarget;
    static bool targetSet;
    static bool jamming;
    static JammerMode currentMode;
    static uint32_t packetsSent;
    static unsigned long lastPacketTime;
    
    // Internal methods
    static void sendInterferencePacket();
    static void sendMimicPacket();
    static void sendDeauthPacket();
    static void randomizeMac();
};
