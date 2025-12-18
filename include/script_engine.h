#pragma once
#include "sd_manager.h"

class ScriptEngine {
public:
    ScriptEngine(SDManager* sd);
    void runScript(String path);

private:
    SDManager* sdManager;
    void parseLine(String line);
};
