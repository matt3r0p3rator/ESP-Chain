# Firmware Launcher - Dual Boot Feature

## Overview
The Firmware Launcher module allows ESP-Chain to boot into secondary firmware (like Bruce) stored in the OTA_1 partition. This gives you the ability to switch between ESP-Chain and another firmware without needing to reflash via USB.

## Features
- ✅ Install secondary firmware from SD card (.bin files)
- ✅ Boot into secondary firmware with one button press
- ✅ View secondary firmware information
- ✅ Erase secondary firmware to free up space
- ✅ Progress indicator during installation
- 🔜 Install from URL (WiFi - coming soon)

## Partition Layout
The new partition scheme allocates:
- **app0 (OTA_0)**: 2MB - ESP-Chain (current firmware)
- **app1 (OTA_1)**: 2MB - Secondary firmware (Bruce, etc.)
- **spiffs**: ~4MB - File storage
- **nvs/otadata**: System data

## How to Use

### 1. Installing Secondary Firmware

#### From SD Card:
1. Download Bruce firmware (or other ESP32-S3 firmware) as a `.bin` file
2. Copy the `.bin` file to your SD card root or `/firmware/` folder
3. In ESP-Chain, navigate to **FW Launcher** module
4. Select **Install from SD**
5. Choose your `.bin` file from the list
6. Wait for installation to complete (progress bar will show)
7. Once complete, you'll see "Secondary FW: Installed" status

#### Recommended Firmware Sources:
- **Bruce**: https://github.com/pr3y/Bruce/releases
  - Download: `Bruce-lilygo-t-display-s3.bin` or appropriate version for your device
  - Size: Usually 1-2MB

### 2. Booting into Secondary Firmware

1. Open **FW Launcher** module
2. Select **Boot Secondary FW**
3. Confirm by selecting **YES**
4. Device will restart and boot into the secondary firmware (Bruce)

### 3. Returning to ESP-Chain

When you're in the secondary firmware (Bruce), you need to reboot the device:
- Bruce typically has a "Restart" or "Reboot" option in its menu
- Or use the hardware reset button
- The device will boot back into ESP-Chain (app0/OTA_0) automatically

### 4. Managing Secondary Firmware

- **FW Info**: View installed secondary firmware details
- **Erase Secondary FW**: Remove the secondary firmware to free up partition space

## Important Notes

### ⚠️ Compatibility
- Only install firmware built for your specific device (ESP32-S3 with appropriate display)
- Firmware must be compatible with the T-Display-S3's hardware configuration
- Incorrect firmware may cause boot loops (solvable by reflashing ESP-Chain via USB)

### ⚠️ Size Limitations
- Secondary firmware must be ≤ 2MB (2,097,152 bytes)
- Most ESP32-S3 firmwares fit within this limit
- If firmware is too large, installation will fail

### ⚠️ Settings Persistence
- ESP-Chain and secondary firmware use separate NVS (settings) partitions
- WiFi credentials and other settings are NOT shared between firmwares
- You'll need to configure each firmware independently

## Troubleshooting

### "No .bin files found on SD card"
- Ensure your .bin file is on the SD card
- Check that the file extension is `.bin` (not `.BIN` or other)
- Try placing the file in `/firmware/` folder

### "Invalid firmware header"
- The .bin file may be corrupted
- Ensure you downloaded the complete file
- Try re-downloading the firmware

### Device boots into wrong firmware
- If you want ESP-Chain: Flash ESP-Chain via USB (will reset boot partition)
- If you want secondary firmware: Use the "Boot Secondary FW" option

### Installation fails
- Check SD card is working properly
- Ensure firmware file is not corrupted
- Verify firmware is < 2MB
- Try erasing secondary firmware first, then reinstall

## Example: Installing Bruce

1. Download Bruce firmware:
   - Visit: https://github.com/pr3y/Bruce/releases
   - Download: `Bruce-lilygo-t-display-s3.bin`

2. Copy to SD card:
   ```
   SD Card Root/
   ├── Bruce-lilygo-t-display-s3.bin
   └── (other files...)
   ```

3. In ESP-Chain:
   - Navigate to **FW Launcher**
   - Select **Install from SD**
   - Choose `Bruce-lilygo-t-display-s3.bin`
   - Wait for installation (~30 seconds)

4. Boot Bruce:
   - Select **Boot Secondary FW**
   - Confirm **YES**
   - Device restarts into Bruce

5. Return to ESP-Chain:
   - In Bruce, select restart/reboot
   - Device boots back to ESP-Chain

## Technical Details

### Partition Operations
- Uses ESP-IDF OTA partition API
- Validates firmware header (0xE9 magic byte)
- Erases partition before writing
- Supports partial writes for large files

### Boot Process
- Uses `esp_ota_set_boot_partition()` to select boot partition
- On reset, ESP32 boots from the last set partition
- ESP-Chain is always the default after power-on reset
- Secondary firmware can be set as boot target temporarily

### File Format
- Standard ESP32 firmware binary format
- Must include bootloader and partition table in single .bin
- Or use app-only .bin that starts at 0x10000

## Future Enhancements
- [ ] WiFi OTA installation from URL
- [ ] Automatic Bruce firmware download
- [ ] Firmware switching without reboot (advanced)
- [ ] Custom boot menu at startup
- [ ] Multiple firmware slots (requires 16MB flash)

## Safety
This feature is designed to be safe:
- ESP-Chain always remains bootable via USB
- Partition operations are atomic
- Invalid firmware won't break ESP-Chain
- Can always reflash via USB if needed
