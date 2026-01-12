#!/bin/bash
# Flash BRUCE firmware to OTA_1 partition
# Usage: ./flash_bruce.sh path/to/bruce.bin

if [ -z "$1" ]; then
    echo "Usage: $0 <bruce_firmware.bin>"
    echo "Example: $0 ~/Downloads/BRUCE-2.5.1-CHEAP_YELLOW_DISPLAY-EN.bin"
    exit 1
fi

BRUCE_FW="$1"
PORT="${2:-/dev/ttyACM0}"
OTA1_OFFSET="0x200000"

if [ ! -f "$BRUCE_FW" ]; then
    echo "Error: Firmware file '$BRUCE_FW' not found!"
    exit 1
fi

SIZE=$(stat -f%z "$BRUCE_FW" 2>/dev/null || stat -c%s "$BRUCE_FW" 2>/dev/null)
SIZE_MB=$(echo "scale=2; $SIZE / 1048576" | bc)

echo "======================================="
echo "Flashing BRUCE to OTA_1 Partition"
echo "======================================="
echo "Firmware: $BRUCE_FW"
echo "Size: $SIZE_MB MB"
echo "Offset: $OTA1_OFFSET (4MB partition)"
echo "Port: $PORT"
echo ""

if (( $(echo "$SIZE > 4194304" | bc -l) )); then
    echo "WARNING: Firmware is larger than 4MB!"
    echo "This may not fit in the partition."
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

esptool.py --chip esp32s3 --port "$PORT" --baud 921600 write_flash "$OTA1_OFFSET" "$BRUCE_FW"

echo ""
echo "======================================="
echo "Done! Reset your device and use"
echo "Firmware Launcher to boot into BRUCE"
echo "======================================="
