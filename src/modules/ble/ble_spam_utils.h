#pragma once
#include <Arduino.h>
#include <vector>

enum SpamType {
    IOS_POPUP,
    ANDROID_PAIR,
    WINDOWS_PAIR,
    SAMSUNG_PAIR,
    KITCHEN_SINK // Cycles through all
};

struct SpamPayload {
    const uint8_t* data;
    size_t length;
};

class BLESpamUtils {
public:
    static void generateRandomMac(uint8_t* mac);
    static std::vector<uint8_t> getAdvertisementData(SpamType type);
};
