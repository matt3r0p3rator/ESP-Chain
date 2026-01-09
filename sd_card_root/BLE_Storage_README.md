# BLE Storage Module

The BLE Storage module enables wireless file transfer between your ESP-Chain device and other devices via Bluetooth Low Energy (BLE). This allows you to manage files on the SD card remotely without physical access.

## Features

- **Wireless File Access**: Browse, download, and upload files via BLE
- **Directory Listing**: Navigate the SD card file system remotely
- **File Transfer**: Bidirectional file transfer (upload/download)
- **Real-time Status**: View connection status and transfer progress
- **Background Operation**: Continues running while using other modules

## How to Use

### On ESP-Chain Device

1. Navigate to "BLE Storage" in the main menu
2. Press OK to start the BLE server
3. The device will show:
   - Connection status
   - Device name (ESP-Chain-Storage)
   - Transfer progress when active
4. The module runs in the background once started
5. Press OK again to stop the server
6. Press BACK to exit

### From Client Device

#### Using Python Client (Included)

A Python client script is provided in the SD card root directory (`ble_storage_client.py`).

**Requirements:**
```bash
pip install bleak
```

**Usage Examples:**

```bash
# List files in root directory
python ble_storage_client.py list

# List files in specific directory  
python ble_storage_client.py list /payloads

# Download a file
python ble_storage_client.py download /config.json config.json

# Upload a file
python ble_storage_client.py upload my_payload.txt /payloads/my_payload.txt
```

#### Using Custom Applications

The BLE service uses the following UUIDs:

- **Service UUID**: `12345678-1234-1234-1234-123456789abc`
- **File List Characteristic**: `12345678-1234-1234-1234-123456789abd` (Read)
- **File Read Characteristic**: `12345678-1234-1234-1234-123456789abe` (Read/Notify)
- **File Write Characteristic**: `12345678-1234-1234-1234-123456789abf` (Write)
- **File Command Characteristic**: `12345678-1234-1234-1234-123456789ac0` (Read/Write)

**Command Protocol:**

1. **List Files**: Write `LIST:/path` to command characteristic, then read from file list characteristic
2. **Download File**: Write `READ:/path/file.txt` to command characteristic, then subscribe to notifications from file read characteristic
3. **Upload File**: Write `WRITE:/path/file.txt` to command characteristic, then write file data to file write characteristic
4. **Stop Transfer**: Write `STOP` to command characteristic

## File Transfer Protocol

### Listing Files

1. Send command: `LIST:/path`
2. Read response from file list characteristic
3. Response format:
   ```
   FILE:filename.txt:1024
   DIR:directory_name
   FILE:another_file.bin:2048
   ```

### Downloading Files

1. Send command: `READ:/path/to/file.txt`
2. Subscribe to notifications on file read characteristic
3. File data will be sent in chunks via notifications
4. Transfer is complete when no more data is received

### Uploading Files

1. Send command: `WRITE:/path/to/file.txt`
2. Wait for confirmation
3. Send file data in chunks to file write characteristic
4. Recommended chunk size: 500 bytes or less

## Security Considerations

- The BLE service is open and does not require authentication
- Only use in trusted environments
- The device name "ESP-Chain-Storage" makes it easily identifiable
- Consider the implications of remote file access

## Troubleshooting

**Device Not Found:**
- Ensure BLE Storage module is running on ESP-Chain
- Check that your client device has BLE capabilities
- Try scanning again - the device may need time to advertise

**Connection Issues:**
- Restart the BLE Storage module
- Check that only one client is connected at a time
- Ensure sufficient battery power on ESP-Chain

**Transfer Problems:**
- Large files may take considerable time over BLE
- SD card must be properly mounted
- Check available space for uploads
- Use smaller chunk sizes if transfers fail

**Permission Errors:**
- Ensure SD card is writable
- Check file paths are correct
- Some system files may be read-only

## Technical Notes

- Maximum recommended file size: 10MB (due to BLE limitations)
- Transfer speed: ~1-5 KB/s (typical for BLE)
- Concurrent connections: 1 client at a time
- Supported file types: All (binary safe)
- Path format: Unix-style paths (`/folder/file.txt`)

## Integration Examples

### Android (Java/Kotlin)
```kotlin
// Connect to device
val bluetoothAdapter = BluetoothAdapter.getDefaultAdapter()
val device = bluetoothAdapter.getRemoteDevice(deviceAddress)
val gatt = device.connectGatt(context, false, gattCallback)

// Send command
val commandService = gatt.getService(UUID.fromString("12345678-1234-1234-1234-123456789abc"))
val commandChar = commandService.getCharacteristic(UUID.fromString("12345678-1234-1234-1234-123456789ac0"))
commandChar.setValue("LIST:/")
gatt.writeCharacteristic(commandChar)
```

### iOS (Swift)
```swift
// Central manager discovery
func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
    if peripheral.name == "ESP-Chain-Storage" {
        self.peripheral = peripheral
        centralManager.connect(peripheral, options: nil)
    }
}

// Send command
let commandUUID = CBUUID(string: "12345678-1234-1234-1234-123456789ac0")
let data = "LIST:/".data(using: .utf8)
peripheral.writeValue(data!, for: commandCharacteristic, type: .withResponse)
```

## Support

For issues or questions about the BLE Storage module:
1. Check the ESP-Chain serial output for debugging information
2. Verify SD card functionality with other modules first
3. Test with the provided Python client before custom implementations