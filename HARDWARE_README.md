# ESP-Chain Hardware Guide

## Button Bindings

The ESP-Chain uses two physical buttons with short-press and long-press actions:

### Button 0 (GPIO 0)
- **Short Press**: Scroll Up / Navigate Up
- **Long Press (500ms)**: Select / Confirm

### Button 14 (GPIO 14)
- **Short Press**: Scroll Down / Navigate Down
- **Long Press (500ms)**: Back / Exit

### Input Mapping
- `0` - Scroll Up
- `1` - Scroll Down
- `2` - Select/Confirm
- `3` - Back/Exit

## Pinout (LilyGo T-Display S3)

### Display (ST7789 - Parallel 8-bit)
| Pin | Function |
|-----|----------|
| 5   | TFT_RST  |
| 6   | TFT_CS   |
| 7   | TFT_DC   |
| 8   | TFT_WR   |
| 9   | TFT_RD   |
| 38  | TFT_BL (Backlight) |
| 39-48 | TFT_D0-D7 (Data Bus) |

**Data Bus Pins**:
- GPIO 39 - TFT_D0
- GPIO 40 - TFT_D1
- GPIO 41 - TFT_D2
- GPIO 42 - TFT_D3
- GPIO 45 - TFT_D4
- GPIO 46 - TFT_D5
- GPIO 47 - TFT_D6
- GPIO 48 - TFT_D7

### SD Card (SPI)
| Pin | Function |
|-----|----------|
| 10  | SD_CS    |
| 11  | SD_MOSI  |
| 12  | SD_SCK   |
| 13  | SD_MISO  |

### User Input
| Pin | Function |
|-----|----------|
| 0   | Button 0 (Up/Select) |
| 14  | Button 14 (Down/Back) |

### Power Management
| Pin | Function |
|-----|----------|
| 17  | External Power Control (NPN) |

## Deep Sleep Configuration

When entering deep sleep:
- GPIO 14 is configured as wake-up button (active LOW)
- GPIO 17 is held LOW to disable external power
- SD card SPI pins are grounded to prevent phantom power draw

## Display Specifications

- **Resolution**: 170x320 pixels
- **Driver**: ST7789
- **Interface**: 8-bit Parallel
- **Color Order**: BGR
- **Inversion**: ON
- **Backlight**: Active HIGH on GPIO 38
