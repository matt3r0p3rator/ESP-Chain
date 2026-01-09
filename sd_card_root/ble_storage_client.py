# BLE Storage Client Example
# This Python script demonstrates how to connect to and use the ESP-Chain BLE Storage module
# Requires: pip install bleak

import asyncio
import sys
from bleak import BleakClient, BleakScanner

# Service and Characteristic UUIDs (must match ESP-Chain)
FILE_TRANSFER_SERVICE_UUID = "12345678-1234-1234-1234-123456789abc"
FILE_LIST_CHARACTERISTIC_UUID = "12345678-1234-1234-1234-123456789abd" 
FILE_READ_CHARACTERISTIC_UUID = "12345678-1234-1234-1234-123456789abe"
FILE_WRITE_CHARACTERISTIC_UUID = "12345678-1234-1234-1234-123456789abf"
FILE_COMMAND_CHARACTERISTIC_UUID = "12345678-1234-1234-1234-123456789ac0"

class BLEStorageClient:
    def __init__(self):
        self.client = None
        self.file_data = bytearray()
        
    async def scan_for_device(self):
        """Scan for ESP-Chain-Storage device"""
        print("Scanning for ESP-Chain-Storage device...")
        devices = await BleakScanner.discover()
        
        for device in devices:
            if device.name and "ESP-Chain-Storage" in device.name:
                print(f"Found device: {device.name} ({device.address})")
                return device.address
        return None
    
    async def connect(self, address):
        """Connect to the BLE device"""
        try:
            self.client = BleakClient(address)
            await self.client.connect()
            print(f"Connected to {address}")
            return True
        except Exception as e:
            print(f"Connection failed: {e}")
            return False
    
    async def disconnect(self):
        """Disconnect from device"""
        if self.client:
            await self.client.disconnect()
            print("Disconnected")
    
    async def list_files(self, path="/"):
        """List files in a directory"""
        if not self.client:
            print("Not connected")
            return []
        
        try:
            # Send list command
            command = f"LIST:{path}"
            await self.client.write_gatt_char(FILE_COMMAND_CHARACTERISTIC_UUID, command.encode())
            
            # Read file list
            await asyncio.sleep(0.5)  # Give device time to process
            file_list_data = await self.client.read_gatt_char(FILE_LIST_CHARACTERISTIC_UUID)
            file_list = file_list_data.decode()
            
            files = []
            for line in file_list.strip().split('\n'):
                if line.startswith("FILE:"):
                    parts = line.split(':')
                    if len(parts) >= 3:
                        files.append({
                            'type': 'file',
                            'name': parts[1],
                            'size': int(parts[2]) if parts[2].isdigit() else 0
                        })
                elif line.startswith("DIR:"):
                    parts = line.split(':')
                    if len(parts) >= 2:
                        files.append({
                            'type': 'directory', 
                            'name': parts[1],
                            'size': 0
                        })
            
            return files
        except Exception as e:
            print(f"Error listing files: {e}")
            return []
    
    async def download_file(self, remote_path, local_path):
        """Download a file from the device"""
        if not self.client:
            print("Not connected")
            return False
        
        try:
            # Start read operation
            command = f"READ:{remote_path}"
            await self.client.write_gatt_char(FILE_COMMAND_CHARACTERISTIC_UUID, command.encode())
            
            # Set up notification handler
            self.file_data = bytearray()
            
            def notification_handler(sender, data):
                self.file_data.extend(data)
            
            await self.client.start_notify(FILE_READ_CHARACTERISTIC_UUID, notification_handler)
            
            print(f"Downloading {remote_path}...")
            
            # Wait for transfer to complete (simple timeout)
            start_time = asyncio.get_event_loop().time()
            last_size = 0
            
            while True:
                await asyncio.sleep(1)
                current_size = len(self.file_data)
                
                if current_size == last_size and current_size > 0:
                    # No new data received, likely complete
                    break
                    
                if asyncio.get_event_loop().time() - start_time > 30:
                    # Timeout after 30 seconds
                    print("Download timeout")
                    break
                    
                last_size = current_size
                print(f"Received {current_size} bytes...")
            
            await self.client.stop_notify(FILE_READ_CHARACTERISTIC_UUID)
            
            # Save to local file
            with open(local_path, 'wb') as f:
                f.write(self.file_data)
            
            print(f"Download complete: {len(self.file_data)} bytes saved to {local_path}")
            return True
            
        except Exception as e:
            print(f"Error downloading file: {e}")
            return False
    
    async def upload_file(self, local_path, remote_path):
        """Upload a file to the device"""
        if not self.client:
            print("Not connected")
            return False
        
        try:
            # Read local file
            with open(local_path, 'rb') as f:
                file_data = f.read()
            
            # Start write operation
            command = f"WRITE:{remote_path}"
            await self.client.write_gatt_char(FILE_COMMAND_CHARACTERISTIC_UUID, command.encode())
            
            await asyncio.sleep(1)  # Give device time to prepare
            
            # Send file data in chunks
            chunk_size = 500  # BLE has limited packet size
            print(f"Uploading {local_path} to {remote_path}...")
            
            for i in range(0, len(file_data), chunk_size):
                chunk = file_data[i:i+chunk_size]
                await self.client.write_gatt_char(FILE_WRITE_CHARACTERISTIC_UUID, chunk)
                print(f"Sent {i+len(chunk)}/{len(file_data)} bytes")
                await asyncio.sleep(0.1)  # Small delay between chunks
            
            print("Upload complete")
            return True
            
        except Exception as e:
            print(f"Error uploading file: {e}")
            return False

async def main():
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python ble_storage_client.py list [path]")
        print("  python ble_storage_client.py download <remote_file> <local_file>")
        print("  python ble_storage_client.py upload <local_file> <remote_file>")
        return
    
    client = BLEStorageClient()
    
    # Find and connect to device
    address = await client.scan_for_device()
    if not address:
        print("ESP-Chain-Storage device not found")
        return
    
    if not await client.connect(address):
        return
    
    try:
        command = sys.argv[1]
        
        if command == "list":
            path = sys.argv[2] if len(sys.argv) > 2 else "/"
            files = await client.list_files(path)
            
            print(f"\\nFiles in {path}:")
            print("-" * 40)
            for file in files:
                type_str = "DIR " if file['type'] == 'directory' else "FILE"
                size_str = "" if file['type'] == 'directory' else f" ({file['size']} bytes)"
                print(f"{type_str:4} {file['name']}{size_str}")
        
        elif command == "download":
            if len(sys.argv) != 4:
                print("Usage: python ble_storage_client.py download <remote_file> <local_file>")
                return
            
            remote_file = sys.argv[2]
            local_file = sys.argv[3]
            await client.download_file(remote_file, local_file)
        
        elif command == "upload":
            if len(sys.argv) != 4:
                print("Usage: python ble_storage_client.py upload <local_file> <remote_file>")
                return
            
            local_file = sys.argv[2]
            remote_file = sys.argv[3]
            await client.upload_file(local_file, remote_file)
        
        else:
            print(f"Unknown command: {command}")
    
    finally:
        await client.disconnect()

if __name__ == "__main__":
    asyncio.run(main())