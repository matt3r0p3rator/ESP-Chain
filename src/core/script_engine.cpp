#include "script_engine.h"

ScriptEngine::ScriptEngine(SDManager* sd) : sdManager(sd) {}

void ScriptEngine::runScript(String path) {
    String content = sdManager->readFile(path);
}

void ScriptEngine::parseLine(String line) {
}
