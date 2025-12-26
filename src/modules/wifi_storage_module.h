#pragma once
#include "module_base.h"
#include "display_manager.h"
#include "sd_manager.h"
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>

extern SDManager sdManager;
extern DisplayManager displayManager;

class WiFiStorageModule : public Module {
    WebServer* server = nullptr;
    bool isRunning = false;
    String ipAddress = "";
    File uploadFile;

    bool deleteFolderRecursively(String path) {
        if (path.endsWith("/")) path = path.substring(0, path.length() - 1);
        
        File root = SD.open(path);
        if (!root) return false;
        if (!root.isDirectory()) {
            root.close();
            return SD.remove(path);
        }
        
        File file = root.openNextFile();
        while (file) {
            String fileName = String(file.name());
            if (fileName.lastIndexOf('/') >= 0) fileName = fileName.substring(fileName.lastIndexOf('/') + 1);
            String fullPath = path + "/" + fileName;
            
            bool isDir = file.isDirectory();
            file.close();
            
            if (isDir) {
                deleteFolderRecursively(fullPath);
            } else {
                SD.remove(fullPath);
            }
            file = root.openNextFile();
        }
        root.close();
        return SD.rmdir(path);
    }

public:
    void init() override {
        // Init is done on enter
    }

    void startServer() {
        if (isRunning) return;

        WiFi.mode(WIFI_AP);
        WiFi.softAP("ESP-Chain-Files", "password");
        ipAddress = WiFi.softAPIP().toString();

        server = new WebServer(80);

        // List files
        server->on("/", HTTP_GET, [this]() {
            if (!sdManager.isMounted()) {
                server->send(500, "text/plain", "SD Card not mounted");
                return;
            }
            
            String path = "/";
            if (server->hasArg("path")) path = server->arg("path");
            if (!path.endsWith("/")) path += "/";

            String html = "<html><head><title>ESP-Chain Files</title>";
            html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
            html += "<style>body{font-family:sans-serif;padding:20px;} ul{list-style:none;padding:0;} li{padding:10px;border-bottom:1px solid #eee;} a{text-decoration:none;color:#007bff;} .del{color:red;margin-left:10px;}</style>";
            html += "</head><body>";
            html += "<h2>ESP-Chain File Manager</h2>";
            html += "<h3>Current Dir: " + path + "</h3>";
            
            if (path != "/") {
                String parent = path.substring(0, path.lastIndexOf('/', path.length() - 2));
                if (parent == "") parent = "/";
                html += "<p><a href='/?path=" + parent + "'>[..] Up</a></p>";
            }

            // New Folder Form
            html += "<form method='GET' action='/mkdir'>";
            html += "<input type='hidden' name='path' value='" + path + "'>";
            html += "<input type='text' name='name' placeholder='New Folder Name'>";
            html += "<input type='submit' value='Create Folder'></form>";
            
            // Upload Form
            html += "<form method='POST' action='/upload?path=" + path + "' enctype='multipart/form-data'>";
            html += "<input type='file' name='upload'><input type='submit' value='Upload File'></form>";

            html += "<hr><ul>";

            // Remove trailing slash for SD.open if not root, as some libs don't like it
            String openPath = path;
            if (openPath.length() > 1 && openPath.endsWith("/")) {
                openPath = openPath.substring(0, openPath.length() - 1);
            }

            File root = SD.open(openPath);
            if (root) {
                if (!root.isDirectory()) {
                    // If path is not a directory
                    root.close();
                } else {
                    File file = root.openNextFile();
                    while (file) {
                        String fileName = String(file.name());
                        if (fileName.lastIndexOf('/') >= 0) fileName = fileName.substring(fileName.lastIndexOf('/') + 1);
                        
                        String fullPath = path + fileName;
                        
                        if (file.isDirectory()) {
                            html += "<li><a href='/?path=" + fullPath + "'><b>[DIR] " + fileName + "</b></a>";
                            html += " <a href='/delete?file=" + fullPath + "' class='del'>[Delete]</a></li>";
                        } else {
                            html += "<li><a href='/download?file=" + fullPath + "'>" + fileName + "</a>";
                            html += " <span style='color:#999;font-size:0.8em'>(" + String(file.size()) + " b)</span>";
                            html += " <a href='/delete?file=" + fullPath + "' class='del'>[Delete]</a></li>";
                        }
                        file = root.openNextFile();
                    }
                    root.close();
                }
            }
            html += "</ul></body></html>";
            server->send(200, "text/html", html);
        });

        // Download
        server->on("/download", HTTP_GET, [this]() {
            if (!server->hasArg("file")) {
                server->send(400, "text/plain", "Missing file arg");
                return;
            }
            String path = server->arg("file");
            if (SD.exists(path)) {
                File file = SD.open(path, FILE_READ);
                server->streamFile(file, "application/octet-stream");
                file.close();
            } else {
                server->send(404, "text/plain", "File not found");
            }
        });

        // Delete
        server->on("/delete", HTTP_GET, [this]() {
            if (!server->hasArg("file")) {
                server->send(400, "text/plain", "Missing file arg");
                return;
            }
            String path = server->arg("file");
            if (SD.exists(path)) {
                SD.remove(path);
                // Redirect back to the folder
                String folder = path.substring(0, path.lastIndexOf('/'));
                if (folder == "") folder = "/";
                server->sendHeader("Location", "/?path=" + folder);
                server->send(303);
            } else {
                server->send(404, "text/plain", "File not found");
            }
        });

        // Create Directory
        server->on("/mkdir", HTTP_GET, [this]() {
            if (!server->hasArg("name") || !server->hasArg("path")) {
                server->send(400, "text/plain", "Missing args");
                return;
            }
            String path = server->arg("path");
            String name = server->arg("name");
            if (!path.endsWith("/")) path += "/";
            String fullPath = path + name;
            
            if (!SD.exists(fullPath)) {
                SD.mkdir(fullPath);
            }
            
            server->sendHeader("Location", "/?path=" + path);
            server->send(303);
        });

        // Upload
        server->on("/upload", HTTP_POST, [this]() {
            String path = "/";
            if (server->hasArg("path")) path = server->arg("path");
            server->sendHeader("Location", "/?path=" + path);
            server->send(303);
        }, [this]() {
            HTTPUpload& upload = server->upload();
            if (upload.status == UPLOAD_FILE_START) {
                String path = "/";
                if (server->hasArg("path")) path = server->arg("path");
                if (!path.endsWith("/")) path += "/";
                
                String filename = upload.filename;
                if (filename.lastIndexOf('/') >= 0) filename = filename.substring(filename.lastIndexOf('/') + 1);
                
                String fullPath = path + filename;
                if (SD.exists(fullPath)) SD.remove(fullPath);
                uploadFile = SD.open(fullPath, FILE_WRITE);
            } else if (upload.status == UPLOAD_FILE_WRITE) {
                if (uploadFile) {
                    uploadFile.write(upload.buf, upload.currentSize);
                }
            } else if (upload.status == UPLOAD_FILE_END) {
                if (uploadFile) {
                    uploadFile.close();
                }
            }
        });

        server->begin();
        isRunning = true;
    }

    void stopServer() {
        if (server) {
            server->stop();
            delete server;
            server = nullptr;
        }
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_OFF);
        isRunning = false;
    }

    void loop() override {
        // UI Loop - nothing to animate
    }
    
    void backgroundLoop() override {
        if (isRunning && server) {
            server->handleClient();
        }
    }
    
    bool isServerRunning() {
        return isRunning;
    }

    bool isBackgroundRunning() override {
        return isRunning;
    }

    String getName() override {
        return "WiFi Storage";
    }

    String getDescription() override {
        return "Web File Manager";
    }

    void drawMenu(DisplayManager* display) override {
        // Update status bar immediately
        display->drawStatusBar("WiFi Storage", display->getBatteryVoltage(), sdManager.isMounted(), isRunning);
        
        display->clearContent();
        display->drawMenuTitle("WiFi Storage");
        
        display->getTFT()->setTextDatum(TL_DATUM);
        display->getTFT()->setTextColor(THEME_TEXT, THEME_BG); 
        
        if (isRunning) {
            display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
            display->getTFT()->drawString("STATUS: RUNNING", 20, 30, 2);
            display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
            
            display->getTFT()->drawString("AP: ESP-Chain-Files", 20, 50, 2);
            display->getTFT()->drawString("Pass: password", 20, 70, 2);
            display->getTFT()->drawString("IP: " + ipAddress, 20, 90, 2);
            
            display->getTFT()->drawString("Btn 1: Stop", 20, 110, 2);
        } else {
            display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
            display->getTFT()->drawString("STOPPED", 20, 60, 2);
            display->getTFT()->setTextColor(THEME_TEXT, THEME_BG);
            
            display->getTFT()->drawString("Btn 1: Start", 20, 100, 2);
        }
        display->getTFT()->drawString("Btn 3: Back", 20, 190, 2);
    }

    bool handleInput(uint8_t button) override {
        if (button == 1) { // Toggle
            if (isRunning) stopServer();
            else startServer();
            drawMenu(&displayManager);
            return true;
        }
        if (button == 3) return false; // Exit
        return true; 
    }
};
