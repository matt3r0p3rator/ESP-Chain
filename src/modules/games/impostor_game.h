#pragma once
#include <Arduino.h>
#include <vector>
#include "module_base.h"
#include "display_manager.h"
#include "sd_manager.h"

enum ImpostorGameState {
    IMPOSTOR_PLAYER_SELECT,
    IMPOSTOR_WORDLIST_SELECT,
    IMPOSTOR_DISTRIBUTE,
    IMPOSTOR_REVEAL
};

class ImpostorGame : public Module {
private:
    ImpostorGameState state;
    int numPlayers;
    int selectedPlayerIndex;
    int currentPlayerRevealing;
    int impostorPlayer;
    String selectedWord;
    std::vector<String> selectedWordlists;
    std::vector<String> availableWordlists;
    int wordlistScrollOffset;
    int selectedWordlistIndex;
    bool wordDistributed;
    unsigned long revealStartTime;
    const int REVEAL_DURATION = 3000; // 3 seconds to view the word
    
    SDManager sdManager;

    // Player selection constants
    const int MIN_PLAYERS = 3;
    const int MAX_PLAYERS = 20;

    void loadWordlists() {
        availableWordlists.clear();
        wordlistScrollOffset = 0;
        selectedWordlistIndex = 0;
        
        // Initialize SD card if needed
        if (!sdManager.isMounted()) {
            if (!sdManager.init()) {
                availableWordlists.push_back("Error: No SD Card");
                return;
            }
        }

        // List wordlist files from /wordlists/ directory
        std::vector<FileEntry> files = sdManager.listDir("/wordlists");
        
        if (files.empty()) {
            availableWordlists.push_back("No wordlists found");
            availableWordlists.push_back("Add .txt files to");
            availableWordlists.push_back("/wordlists/ on SD");
        } else {
            for (const auto& file : files) {
                if (!file.isDirectory && file.name.endsWith(".txt")) {
                    availableWordlists.push_back(file.name);
                }
            }
            if (availableWordlists.empty()) {
                availableWordlists.push_back("No .txt files found");
            }
        }
    }

    String getRandomWord() {
        if (!sdManager.isMounted() || selectedWordlists.empty()) {
            return "ERROR";
        }

        // Collect words from all selected wordlists
        std::vector<String> words;
        
        for (const auto& wordlist : selectedWordlists) {
            String path = "/wordlists/" + wordlist;
            String content = sdManager.readFile(path);
            
            if (content.length() == 0) continue;

            // Parse words (one per line)
            int startIdx = 0;
            for (int i = 0; i < content.length(); i++) {
                if (content[i] == '\n' || content[i] == '\r') {
                    if (i > startIdx) {
                        String word = content.substring(startIdx, i);
                        word.trim();
                        if (word.length() > 0) {
                            words.push_back(word);
                        }
                    }
                    startIdx = i + 1;
                }
            }
            // Add last word if file doesn't end with newline
            if (startIdx < content.length()) {
                String word = content.substring(startIdx);
                word.trim();
                if (word.length() > 0) {
                    words.push_back(word);
                }
            }
        }

        if (words.empty()) {
            return "NO_WORDS";
        }

        // Return random word
        return words[random(0, words.size())];
    }

    void distributeWords() {
        randomSeed(millis());
        
        // Pick random impostor
        impostorPlayer = random(0, numPlayers);
        
        // Pick random word
        selectedWord = getRandomWord();
        
        // Fallback to test word if loading failed
        if (selectedWord.length() == 0 || selectedWord == "ERROR" || selectedWord == "EMPTY" || selectedWord == "NO_WORDS") {
            selectedWord = "TESTWORD";
        }
        
        wordDistributed = true;
        currentPlayerRevealing = 0;
        state = IMPOSTOR_DISTRIBUTE;
    }

    void drawPlayerSelection(DisplayManager* display) {
        display->clearContent();
        
        TFT_eSPI* tft = display->getTFT();
        tft->setTextDatum(TC_DATUM);
        
        // Title
        tft->setTextColor(THEME_TEXT, THEME_BG);
        tft->setTextSize(1);
        tft->drawString("Select Players", 160, 30, 4);
        
        // Player count display
        tft->setTextSize(1);
        tft->setTextColor(THEME_ACCENT, THEME_BG);
        tft->drawString(String(numPlayers), 160, 70, 7);
        
        // Instructions
        tft->setTextSize(1);
        tft->setTextColor(THEME_TEXT, THEME_BG);
        tft->drawString("Btn0/1: Change", 160, 130, 2);
        tft->drawString("Long Press 0: Next", 160, 150, 2);
    }

    void drawWordlistSelection(DisplayManager* display) {
        display->clearMenu();
        
        // Check if we have wordlists
        if (availableWordlists.empty()) {
            TFT_eSPI* tft = display->getTFT();
            tft->setTextDatum(TC_DATUM);
            tft->setTextColor(TFT_RED, THEME_BG);
            tft->drawString("No wordlists!", 160, 70, 2);
            tft->setTextColor(THEME_TEXT, THEME_BG);
            tft->drawString("Add .txt files to", 160, 95, 2);
            tft->drawString("/wordlists/ on SD", 160, 115, 2);
            return;
        }
        
        // Total items = wordlists + 1 (Next button)
        int totalItems = availableWordlists.size() + 1;
        int itemsPerPage = 5;
        int displayCount = 0;
        
        for (int i = wordlistScrollOffset; i < totalItems && displayCount < itemsPerPage; i++, displayCount++) {
            bool highlighted = (i == selectedWordlistIndex);
            
            if (i < availableWordlists.size()) {
                // Regular wordlist item
                bool checked = false;
                
                // Check if this wordlist is selected
                for (const auto& sel : selectedWordlists) {
                    if (sel == availableWordlists[i]) {
                        checked = true;
                        break;
                    }
                }
                
                // Create display name with checkbox
                String displayName = checked ? "[X] " : "[ ] ";
                displayName += availableWordlists[i];
                
                display->drawMenuItem(displayName, displayCount, highlighted);
            } else {
                // Next button
                String nextText = ">>> Next (";
                nextText += String(selectedWordlists.size());
                nextText += " selected)";
                display->drawMenuItem(nextText, displayCount, highlighted);
            }
        }
        
        // Draw scroll indicator if needed
        if (totalItems > itemsPerPage) {
            display->drawScrollBar(totalItems, wordlistScrollOffset, itemsPerPage);
        }
        
        display->updateMenu();
    }

    void drawDistribute(DisplayManager* display) {
        TFT_eSPI* tft = display->getTFT();
        
        // Clear entire screen area
        tft->fillRect(0, 20, 320, 150, THEME_BG);
        tft->setTextDatum(TC_DATUM);
        
        // Title
        tft->setTextColor(THEME_TEXT, THEME_BG);
        tft->setTextSize(1);
        tft->drawString("Player " + String(currentPlayerRevealing + 1), 160, 40, 4);
        
        // Instructions
        tft->setTextSize(1);
        tft->drawString("Long press Btn 0", 160, 80, 2);
        tft->drawString("to reveal your word", 160, 100, 2);
        
        // Warning
        tft->setTextColor(THEME_ACCENT, THEME_BG);
        tft->drawString("(Keep it secret!)", 160, 135, 2);
    }

    void drawReveal(DisplayManager* display) {
        TFT_eSPI* tft = display->getTFT();
        
        // Clear entire screen area
        tft->fillRect(0, 20, 320, 150, THEME_BG);
        tft->setTextDatum(TC_DATUM);
        
        // Player number
        tft->setTextColor(THEME_TEXT, THEME_BG);
        tft->setTextSize(1);
        String playerText = "Player ";
        playerText += String(currentPlayerRevealing + 1);
        tft->drawString(playerText, 160, 40, 2);
        
        // Show word or impostor message
        if (currentPlayerRevealing == impostorPlayer) {
            tft->setTextColor(TFT_RED, THEME_BG);
            tft->setTextSize(1);
            tft->setFreeFont(&FreeSans24pt7b);
            tft->drawString("IMPOSTOR", 160, 75);
            tft->setTextSize(1);
            tft->setTextColor(THEME_TEXT, THEME_BG);
            tft->drawString("No word for you!", 160, 120, 2);
            tft->drawString("Blend in!", 160, 140, 2);
        } else {
            // Show the word
            tft->setTextColor(TFT_YELLOW, THEME_BG);
            tft->setTextSize(1);
            tft->setFreeFont(&FreeSansBold24pt7b);
            tft->drawString(selectedWord, 160, 85);
            
            tft->setTextSize(1);
            tft->setTextColor(THEME_TEXT, THEME_BG);
            tft->drawString("Remember!", 160, 135, 2);
        }
    }

public:
    ImpostorGame() : 
        state(IMPOSTOR_PLAYER_SELECT),
        numPlayers(5),
        selectedPlayerIndex(0),
        currentPlayerRevealing(0),
        impostorPlayer(-1),
        wordDistributed(false),
        wordlistScrollOffset(0),
        selectedWordlistIndex(0) {}

    void init() override {
        state = IMPOSTOR_PLAYER_SELECT;
        numPlayers = 5;
        wordDistributed = false;
        currentPlayerRevealing = 0;
        impostorPlayer = -1;
        selectedPlayerIndex = 0;
        wordlistScrollOffset = 0;
        selectedWordlistIndex = 0;
        selectedWordlists.clear();
        loadWordlists();
    }

    void loop() override {
        // Auto-advance after reveal timeout
        if (state == IMPOSTOR_REVEAL) {
            if (millis() - revealStartTime > REVEAL_DURATION) {
                // Move to next player or finish
                currentPlayerRevealing++;
                extern DisplayManager displayManager;
                if (currentPlayerRevealing >= numPlayers) {
                    // Game complete - show summary or restart
                    state = IMPOSTOR_PLAYER_SELECT;
                    init();
                } else {
                    state = IMPOSTOR_DISTRIBUTE;
                }
                drawMenu(&displayManager);
            }
        }
    }

    String getName() override {
        return "Impostor";
    }

    String getDescription() override {
        return "Social deduction word game";
    }

    void drawMenu(DisplayManager* display) override {
        switch (state) {
            case IMPOSTOR_PLAYER_SELECT:
                drawPlayerSelection(display);
                break;
            case IMPOSTOR_WORDLIST_SELECT:
                drawWordlistSelection(display);
                break;
            case IMPOSTOR_DISTRIBUTE:
                drawDistribute(display);
                break;
            case IMPOSTOR_REVEAL:
                drawReveal(display);
                break;
        }
    }

    bool handleInput(uint8_t button) override {
        extern DisplayManager displayManager;
        
        switch (state) {
            case IMPOSTOR_PLAYER_SELECT:
                if (button == 0) { // UP
                    if (numPlayers < MAX_PLAYERS) {
                        numPlayers++;
                        drawMenu(&displayManager);
                    }
                } else if (button == 1) { // DOWN
                    if (numPlayers > MIN_PLAYERS) {
                        numPlayers--;
                        drawMenu(&displayManager);
                    }
                } else if (button == 2) { // SELECT
                    state = IMPOSTOR_WORDLIST_SELECT;
                    drawMenu(&displayManager);
                } else if (button == 3) { // BACK
                    return false; // Exit module
                }
                break;

            case IMPOSTOR_WORDLIST_SELECT:
                if (button == 0) { // UP - Scroll up
                    int totalItems = availableWordlists.size() + 1; // +1 for Next button
                    if (selectedWordlistIndex > 0) {
                        selectedWordlistIndex--;
                    } else {
                        // Wraparound to last item
                        selectedWordlistIndex = totalItems - 1;
                    }
                    
                    // Adjust scroll offset
                    if (selectedWordlistIndex < wordlistScrollOffset) {
                        wordlistScrollOffset = selectedWordlistIndex;
                    } else if (selectedWordlistIndex >= wordlistScrollOffset + 5) {
                        wordlistScrollOffset = selectedWordlistIndex - 4;
                    }
                    drawMenu(&displayManager);
                } else if (button == 1) { // DOWN - Scroll down
                    int totalItems = availableWordlists.size() + 1; // +1 for Next button
                    if (selectedWordlistIndex < totalItems - 1) {
                        selectedWordlistIndex++;
                    } else {
                        // Wraparound to first item
                        selectedWordlistIndex = 0;
                    }
                    
                    // Adjust scroll offset
                    if (selectedWordlistIndex < wordlistScrollOffset) {
                        wordlistScrollOffset = selectedWordlistIndex;
                    } else if (selectedWordlistIndex >= wordlistScrollOffset + 5) {
                        wordlistScrollOffset = selectedWordlistIndex - 4;
                    }
                    drawMenu(&displayManager);
                } else if (button == 2) { // SELECT - Toggle checkbox or proceed
                    if (selectedWordlistIndex < (int)availableWordlists.size()) {
                        // Toggling a wordlist checkbox
                        if (!availableWordlists[selectedWordlistIndex].startsWith("Error") &&
                            !availableWordlists[selectedWordlistIndex].startsWith("No")) {
                            
                            String wordlist = availableWordlists[selectedWordlistIndex];
                            bool found = false;
                            
                            for (int i = 0; i < selectedWordlists.size(); i++) {
                                if (selectedWordlists[i] == wordlist) {
                                    selectedWordlists.erase(selectedWordlists.begin() + i);
                                    found = true;
                                    break;
                                }
                            }
                            
                            if (!found) {
                                selectedWordlists.push_back(wordlist);
                            }
                            drawMenu(&displayManager);
                        }
                    } else {
                        // Next button pressed
                        if (selectedWordlists.size() > 0) {
                            distributeWords();
                            drawMenu(&displayManager);
                        }
                    }
                } else if (button == 3) { // BACK
                    state = IMPOSTOR_PLAYER_SELECT;
                    drawMenu(&displayManager);
                }
                break;

            case IMPOSTOR_DISTRIBUTE:
                if (button == 2) { // SELECT
                    state = IMPOSTOR_REVEAL;
                    revealStartTime = millis();
                    drawMenu(&displayManager);
                } else if (button == 3) { // BACK
                    state = IMPOSTOR_PLAYER_SELECT;
                    init();
                    drawMenu(&displayManager);
                }
                break;

            case IMPOSTOR_REVEAL:
                // Can manually advance with SELECT
                if (button == 2) {
                    currentPlayerRevealing++;
                    if (currentPlayerRevealing >= numPlayers) {
                        state = IMPOSTOR_PLAYER_SELECT;
                        init();
                    } else {
                        state = IMPOSTOR_DISTRIBUTE;
                    }
                    drawMenu(&displayManager);
                } else if (button == 3) { // BACK
                    state = IMPOSTOR_PLAYER_SELECT;
                    init();
                    drawMenu(&displayManager);
                }
                break;
        }
        return true;
    }
};
