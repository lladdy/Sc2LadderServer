#pragma once

#include "Types.h"

#include <string>
#include <document.h>

class LadderBotsLoader {
public:
    void Load(std::string file);
    std::unique_ptr<std::map<std::string, BotConfig>> GetBotConfigs();
    std::unique_ptr<std::vector<std::string>> GetMaps();
private:
    rapidjson::Document doc;
};