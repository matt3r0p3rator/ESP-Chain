#include "ble_storage_module.h"

void BLEStorageModule::startBLEServer() {
    if (currentState == RUNNING) return;
    
    currentState = STARTING;
    statusMessage = "Initializing...";
    
    try {
        // Initialize BLE
        BLEDevice::init(deviceName.c_str());
        
        // Create BLE Server
        pServer = BLEDevice::createServer();
        pServer->setCallbacks(new BLEStorageServerCallbacks(this));
        
        // Setup service and characteristics
        setupBLEService();
        
        // Start advertising
        BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
        pAdvertising->addServiceUUID(FILE_TRANSFER_SERVICE_UUID);
        pAdvertising->setScanResponse(true);
        pAdvertising->setMinPreferred(0x06);
        pAdvertising->setMinPreferred(0x12);
        BLEDevice::startAdvertising();
        
        currentState = RUNNING;
        statusMessage = "Server running";
        isRunning = true;
        
    } catch (...) {
        currentState = ERROR;
        statusMessage = "Failed to start BLE server";
        isRunning = false;
    }
}

void BLEStorageModule::stopBLEServer() {
    if (currentState != RUNNING) return;
    
    // Close any open file
    if (currentFile) {
        currentFile.close();
    }
    fileTransferActive = false;
    
    // Stop BLE
    if (pServer) {
        BLEDevice::stopAdvertising();
        pServer->disconnect(0);
        BLEDevice::deinit();
        pServer = nullptr;
        pService = nullptr;
        pFileListChar = nullptr;
        pFileReadChar = nullptr;
        pFileWriteChar = nullptr;
        pFileCommandChar = nullptr;
    }
    
    currentState = STOPPED;
    statusMessage = "Server stopped";
    isRunning = false;
    deviceConnected = false;
    oldDeviceConnected = false;
}

void BLEStorageModule::setupBLEService() {
    // Create service
    pService = pServer->createService(FILE_TRANSFER_SERVICE_UUID);
    
    // File list characteristic (read)
    pFileListChar = pService->createCharacteristic(
        FILE_LIST_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ
    );
    
    // File read characteristic (read/notify)
    pFileReadChar = pService->createCharacteristic(
        FILE_READ_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pFileReadChar->addDescriptor(new BLE2902());
    
    // File write characteristic (write)
    pFileWriteChar = pService->createCharacteristic(
        FILE_WRITE_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pFileWriteChar->setCallbacks(new FileWriteCallbacks(this));
    
    // File command characteristic (read/write)
    pFileCommandChar = pService->createCharacteristic(
        FILE_COMMAND_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    );
    pFileCommandChar->setCallbacks(new FileCommandCallbacks(this));
    
    // Start service
    pService->start();
}

void BLEStorageModule::handleConnectionState() {
    // Handle connection/disconnection
    if (deviceConnected != oldDeviceConnected) {
        if (deviceConnected) {
            connectedClients++;
            // Client connected
        } else {
            connectedClients = 0;
            // Client disconnected - restart advertising
            if (currentState == RUNNING) {
                delay(500);
                pServer->startAdvertising();
            }
        }
        oldDeviceConnected = deviceConnected;
    }
    
    // Continue file transfer if active
    if (fileTransferActive) {
        continueFileTransfer();
    }
}

void BLEStorageModule::updateUI() {
    // Update UI every second
    if (millis() - lastUIUpdate > 1000) {
        lastUIUpdate = millis();
        // UI is updated in drawMenu()
    }
}

String BLEStorageModule::listFiles(String path) {
    String fileList = "";
    
    if (!sdManager.isMounted()) {
        return "ERROR: SD card not mounted";
    }
    
    File root = SD.open(path);
    if (!root || !root.isDirectory()) {
        return "ERROR: Invalid directory";
    }
    
    File file = root.openNextFile();
    while (file) {
        String fileName = String(file.name());
        if (fileName.startsWith(path)) {
            fileName = fileName.substring(path.length());
            if (fileName.startsWith("/")) {
                fileName = fileName.substring(1);
            }
        }
        
        if (file.isDirectory()) {
            fileList += "DIR:" + fileName + "\n";
        } else {
            fileList += "FILE:" + fileName + ":" + String(file.size()) + "\n";
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();
    
    return fileList;
}

void BLEStorageModule::handleFileCommand(String command) {
    if (command.startsWith("LIST:")) {
        String path = command.substring(5);
        if (path.length() == 0) path = "/";
        
        String fileList = listFiles(path);
        pFileListChar->setValue(fileList.c_str());
        
    } else if (command.startsWith("READ:")) {
        String filePath = command.substring(5);
        startFileRead(filePath);
        
    } else if (command.startsWith("WRITE:")) {
        String filePath = command.substring(6);
        startFileWrite(filePath);
        
    } else if (command.equals("STOP")) {
        if (currentFile) {
            currentFile.close();
        }
        fileTransferActive = false;
        transferredBytes = 0;
        totalBytes = 0;
        currentFilePath = "";
    }
}

void BLEStorageModule::startFileRead(String filePath) {
    if (!sdManager.isMounted()) {
        pFileCommandChar->setValue("ERROR: SD not mounted");
        return;
    }
    
    if (currentFile) {
        currentFile.close();
    }
    
    currentFile = SD.open(filePath);
    if (!currentFile) {
        pFileCommandChar->setValue("ERROR: Cannot open file");
        return;
    }
    
    if (currentFile.isDirectory()) {
        currentFile.close();
        pFileCommandChar->setValue("ERROR: Cannot read directory");
        return;
    }
    
    currentFilePath = filePath;
    fileTransferActive = true;
    transferredBytes = 0;
    totalBytes = currentFile.size();
    
    pFileCommandChar->setValue("OK: Starting read");
}

void BLEStorageModule::startFileWrite(String filePath) {
    if (!sdManager.isMounted()) {
        pFileCommandChar->setValue("ERROR: SD not mounted");
        return;
    }
    
    if (currentFile) {
        currentFile.close();
    }
    
    // Create directories if needed
    int lastSlash = filePath.lastIndexOf('/');
    if (lastSlash > 0) {
        String dirPath = filePath.substring(0, lastSlash);
        // Create directory structure (simplified)
        SD.mkdir(dirPath);
    }
    
    currentFile = SD.open(filePath, FILE_WRITE);
    if (!currentFile) {
        pFileCommandChar->setValue("ERROR: Cannot create file");
        return;
    }
    
    currentFilePath = filePath;
    fileTransferActive = true;
    transferredBytes = 0;
    totalBytes = 0; // Unknown for writes
    
    pFileCommandChar->setValue("OK: Ready for write");
}

void BLEStorageModule::continueFileTransfer() {
    if (!fileTransferActive || !currentFile) {
        return;
    }
    
    // Handle read operations
    if (currentFile.available() > 0) {
        uint8_t buffer[512];
        size_t bytesToRead = min((size_t)currentFile.available(), sizeof(buffer));
        size_t bytesRead = currentFile.read(buffer, bytesToRead);
        
        if (bytesRead > 0) {
            pFileReadChar->setValue(buffer, bytesRead);
            pFileReadChar->notify();
            transferredBytes += bytesRead;
            
            // Small delay to prevent overwhelming the client
            delay(50);
        }
        
        if (!currentFile.available()) {
            // Transfer complete
            currentFile.close();
            fileTransferActive = false;
            pFileCommandChar->setValue("OK: Read complete");
        }
    }
}