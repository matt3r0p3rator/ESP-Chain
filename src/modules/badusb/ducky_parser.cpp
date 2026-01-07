#include "ducky_parser.h"
#include <SD.h>

DuckyParser::DuckyParser(USBHIDKeyboard* keyboard) : _keyboard(keyboard) {}

void DuckyParser::parseFile(String filePath) {
    if (!SD.exists(filePath)) return;
    
    File file = SD.open(filePath);
    if (!file) return;

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        processLine(line);
    }
    file.close();
}

void DuckyParser::processMultiModifier(String line) {
    // Parse a line with multiple modifiers like "CTRL GUI h" or "SHIFT ALT F4"
    line.toUpperCase();
    line.trim();
    
    // Split the line into tokens
    String tokens[10];
    int tokenCount = 0;
    int start = 0;
    
    for (int i = 0; i <= line.length(); i++) {
        if (i == line.length() || line.charAt(i) == ' ') {
            if (i > start) {
                tokens[tokenCount++] = line.substring(start, i);
                if (tokenCount >= 10) break;
            }
            start = i + 1;
        }
    }
    
    if (tokenCount == 0) return;
    
    // Press all modifiers
    for (int i = 0; i < tokenCount - 1; i++) {
        if (tokens[i] == "CTRL" || tokens[i] == "CONTROL") {
            _keyboard->press(KEY_LEFT_CTRL);
        } else if (tokens[i] == "SHIFT") {
            _keyboard->press(KEY_LEFT_SHIFT);
        } else if (tokens[i] == "ALT") {
            _keyboard->press(KEY_LEFT_ALT);
        } else if (tokens[i] == "GUI" || tokens[i] == "WINDOWS") {
            _keyboard->press(KEY_LEFT_GUI);
        }
    }
    
    delay(50);
    
    // Press the final key
    String finalKey = tokens[tokenCount - 1];
    uint8_t k = getKeyCode(finalKey);
    if (k) {
        _keyboard->press(k);
    } else if (finalKey.length() == 1) {
        char c = finalKey.charAt(0);
        if (c >= 'A' && c <= 'Z') {
             _keyboard->press(tolower(c));
        }
    }
    
    delay(50);
    _keyboard->releaseAll();
}

void DuckyParser::processLine(String line) {
    if (line.length() == 0) return;
    
    int spaceIndex = line.indexOf(' ');
    String command = (spaceIndex == -1) ? line : line.substring(0, spaceIndex);
    String args = (spaceIndex == -1) ? "" : line.substring(spaceIndex + 1);
    
    command.toUpperCase();
    
    // Check if this is a multi-modifier combination (e.g., CTRL GUI h, SHIFT ALT F4)
    if (args.length() > 0) {
        args.trim();
        String argsUpper = args;
        argsUpper.toUpperCase();
        
        // Check if args contains modifier keywords
        if ((command == "CTRL" || command == "CONTROL" || command == "ALT" || 
             command == "SHIFT" || command == "GUI" || command == "WINDOWS") &&
            (argsUpper.indexOf("CTRL") >= 0 || argsUpper.indexOf("CONTROL") >= 0 ||
             argsUpper.indexOf("ALT") >= 0 || argsUpper.indexOf("SHIFT") >= 0 ||
             argsUpper.indexOf("GUI") >= 0 || argsUpper.indexOf("WINDOWS") >= 0)) {
            
            // This is a multi-modifier combo, handle it specially
            processMultiModifier(command + " " + args);
            if (defaultDelay > 0) delay(defaultDelay);
            return;
        }
    }

    if (command == "REM") {
        return;
    } else if (command == "DELAY") {
        delay(args.toInt());
    } else if (command == "DEFAULTDELAY" || command == "DEFAULT_DELAY") {
        defaultDelay = args.toInt();
    } else if (command == "STRING") {
        _keyboard->print(args);
    } else if (command == "GUI" || command == "WINDOWS") {
        if (args.length() > 0) {
            // Handle the argument first
            args.trim();
            uint8_t k = getKeyCode(args);
            
            // Press GUI key first
            _keyboard->press(KEY_LEFT_GUI);
            delay(50);
            
            // Then press the target key
            if (k) {
                _keyboard->press(k);
            } else if (args.length() == 1) {
                char c = args.charAt(0);
                if (c >= 'a' && c <= 'z') {
                    _keyboard->press(c);
                } else if (c >= 'A' && c <= 'Z') {
                    _keyboard->press(tolower(c));
                }
            }
            
            delay(50); // Hold both keys
            _keyboard->releaseAll();
        } else {
            // Just Windows key alone
            _keyboard->press(KEY_LEFT_GUI);
            delay(100); // Longer delay to ensure registration
            _keyboard->releaseAll(); 
            delay(50);
        }
    } else if (command == "APP" || command == "MENU") {
        // _keyboard->press(KEY_MENU); // Not always defined, check library
    } else if (command == "SHIFT") {
        if (args.length() > 0) {
            _keyboard->press(KEY_LEFT_SHIFT);
            // Handle special keys or characters
             uint8_t k = getKeyCode(args);
             if (k) _keyboard->press(k);
             else _keyboard->print(args);
             delay(50); // Increased delay before releasing keys
             _keyboard->releaseAll();
        } else {
            _keyboard->press(KEY_LEFT_SHIFT);
            delay(50); // Increased delay to ensure key press is registered
            _keyboard->releaseAll();
        }
    } else if (command == "ALT") {
        if (args.length() > 0) {
            _keyboard->press(KEY_LEFT_ALT);
            uint8_t k = getKeyCode(args);
            if (k) _keyboard->press(k);
            else _keyboard->print(args);
            delay(10);
            _keyboard->releaseAll();
        } else {
            _keyboard->press(KEY_LEFT_ALT);
            delay(10);
            _keyboard->releaseAll();
        }
    } else if (command == "CTRL" || command == "CONTROL") {
        if (args.length() > 0) {
            _keyboard->press(KEY_LEFT_CTRL);
            uint8_t k = getKeyCode(args);
            if (k) _keyboard->press(k);
            else _keyboard->print(args);
            delay(10);
            _keyboard->releaseAll();
        } else {
            _keyboard->press(KEY_LEFT_CTRL);
            delay(10);
            _keyboard->releaseAll();
        }
    } else {
        uint8_t key = getKeyCode(command);
        if (key != 0) {
            _keyboard->press(key);
            delay(10);
            _keyboard->releaseAll();
        }
    }
    
    if (defaultDelay > 0) delay(defaultDelay);
}

uint8_t DuckyParser::getKeyCode(String key) {
    key.toUpperCase();
    if (key == "ENTER") return KEY_RETURN;
    if (key == "UP" || key == "UPARROW") return KEY_UP_ARROW;
    if (key == "DOWN" || key == "DOWNARROW") return KEY_DOWN_ARROW;
    if (key == "LEFT" || key == "LEFTARROW") return KEY_LEFT_ARROW;
    if (key == "RIGHT" || key == "RIGHTARROW") return KEY_RIGHT_ARROW;
    if (key == "BACKSPACE") return KEY_BACKSPACE;
    if (key == "TAB") return KEY_TAB;
    if (key == "CAPSLOCK") return KEY_CAPS_LOCK;
    if (key == "DELETE") return KEY_DELETE;
    if (key == "END") return KEY_END;
    if (key == "ESC" || key == "ESCAPE") return KEY_ESC;
    if (key == "HOME") return KEY_HOME;
    if (key == "INSERT") return KEY_INSERT;
    if (key == "PAGEUP") return KEY_PAGE_UP;
    if (key == "PAGEDOWN") return KEY_PAGE_DOWN;
    // if (key == "PRINTSCREEN") return 0xCE; // PrintScreen
    if (key == "SPACE") return ' ';
    if (key == "PLUS") return '+';
    if (key == "MINUS") return '-';
    // if (key == "VOLUMEUP") return 0x80; // Volume Up
    // if (key == "VOLUMEDOWN") return 0x81; // Volume Down
    
    if (key.startsWith("F")) {
        int fNum = key.substring(1).toInt();
        if (fNum >= 1 && fNum <= 12) return KEY_F1 + (fNum - 1);
    }
    
    return 0;
}

void DuckyParser::pressKey(String key) {
    uint8_t k = getKeyCode(key);
    if (k) {
        _keyboard->press(k);
        delay(10);
        _keyboard->releaseAll();
    }
}

void DuckyParser::pressCombination(String modifiers, String key) {
    // Simplified handling in processLine
}
