#include "nrf24_driver.h"

// SPI Pins matching SD Card
#define SPI_MOSI 11
#define SPI_MISO 13
#define SPI_SCK  12
#define NRF_SPI_CS_PIN 2 // Chip Select pin for NRF24
#define NRF_CE_PIN 3  // Chip Enable pin for NRF24

NRF24Driver::NRF24Driver() : _radio(NRF_CE_PIN, NRF_SPI_CS_PIN) {
}

bool NRF24Driver::begin() {
    // Initialize SPI with the shared pins
    // Note: SPI.begin() handles the pin assignment for the SPI bus
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, NRF_SPI_CS_PIN);
    
    // Initialize radio with the SPI bus
    if (!_radio.begin(&SPI)) {
        return false;
    }
    
    return true;
}

RF24& NRF24Driver::getRadio() {
    return _radio;
}

