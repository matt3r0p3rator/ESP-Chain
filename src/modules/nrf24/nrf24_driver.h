#pragma once
#include <Arduino.h>
#include <RF24.h>
#include <SPI.h>

class NRF24Driver {
public:
    NRF24Driver();
    bool begin();
    RF24& getRadio();

private:
    RF24 _radio;
};
