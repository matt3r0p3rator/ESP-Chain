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

void DuckyParser::processLine(String line) {
    if (line.length() == 0) return;
    
    int spaceIndex = line.indexOf(' ');
    String command = (spaceIndex == -1) ? line : line.substring(0, spaceIndex);
    String args = (spaceIndex == -1) ? "" : line.substring(spaceIndex + 1);
    
    command.toUpperCase();

    if (command == "REM") {
        return;
    } else if (command == "DELAY") {
        delay(args.toInt());
    } else if (command == "DEFAULTDELAY" || command == "DEFAULT_DELAY") {
        defaultDelay = args.toInt();
    } else if (command == "STRING") {
        _keyboard->print(args);
    } else if (command == "GUI" || command == "WINDOWS" || command == "SEARCH") {
        if (args.length() > 0) {
            _keyboard->press(KEY_LEFT_GUI);
            if (args.length() == 1) {
                _keyboard->print(args); // e.g. GUI r
            } else {
                uint8_t k = getKeyCode(args);
                if (k) _keyboard->press(k);
            }
            delay(10);
            _keyboard->releaseAll();
        } else {
            _keyboard->press(KEY_LEFT_GUI);
            delay(10);
            _keyboard->releaseAll();
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
             delay(10);
             _keyboard->releaseAll();
        } else {
            _keyboard->press(KEY_LEFT_SHIFT);
            delay(10);
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
    if (key == "PRINTSCREEN") return 0xCE; // PrintScreen
    if (key == "SPACE") return ' ';
    
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
