#include "LadderBotsLoader.h"


#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

void LadderBotsLoader::Load(std::string file) {
    // todo error handling
    if (file.length() < 1)
    {
        return;
    }
    std::ifstream t(file);
    std::stringstream buffer;
    buffer << t.rdbuf();
    std::string BotConfigString = buffer.str();
    rapidjson::Document doc;
    bool parsingFailed = doc.Parse(file.c_str()).HasParseError();
    if (parsingFailed)
    {
        std::cerr << "Unable to parse bot config file: " << file << std::endl;
        return;
    }
}

std::string LadderBotsLoader::GerneratePlayerId(size_t Length)
{
    static const char hexdigit[16] = { '0', '1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
    std::string outstring;
    if (Length < 1)
    {
        return outstring;

    }
    --Length;
    for (int i = 0; i < Length; ++i)
    {
        outstring.append(1, hexdigit[rand() % sizeof hexdigit]);
    }
    return outstring;
}

std::unique_ptr<std::map<std::string, BotConfig>> LadderBotsLoader::GetBotConfigs() {
    // todo error handling
    std::unique_ptr<std::map<std::string, BotConfig>> botConfigs(new std::map<std::string, BotConfig>());

    if (doc.HasMember("Bots") && doc["Bots"].IsObject())
    {
        const rapidjson::Value & Bots = doc["Bots"];
        for (auto itr = Bots.MemberBegin(); itr != Bots.MemberEnd(); ++itr)
        {
            BotConfig NewBot;
            NewBot.BotName = itr->name.GetString();
            const rapidjson::Value &val = itr->value;

            if (val.HasMember("Race") && val["Race"].IsString())
            {
                NewBot.Race = GetRaceFromString(val["Race"].GetString());
            }
            else
            {
                std::cerr << "Unable to parse race for bot " << NewBot.BotName << std::endl;
                continue;
            }
            if (val.HasMember("Type") && val["Type"].IsString())
            {
                NewBot.Type = GetTypeFromString(val["Type"].GetString());
            }
            else
            {
                std::cerr << "Unable to parse type for bot " << NewBot.BotName << std::endl;
                continue;
            }
            if (NewBot.Type != DefaultBot)
            {
                if (val.HasMember("RootPath") && val["RootPath"].IsString())
                {
                    NewBot.RootPath = val["RootPath"].GetString();
                    if (NewBot.RootPath.back() != '/')
                    {
                        NewBot.RootPath += '/';
                    }
                }
                else
                {
                    std::cerr << "Unable to parse root path for bot " << NewBot.BotName << std::endl;
                    continue;
                }
                if (val.HasMember("FileName") && val["FileName"].IsString())
                {
                    NewBot.FileName = val["FileName"].GetString();
                }
                else
                {
                    std::cerr << "Unable to parse file name for bot " << NewBot.BotName << std::endl;
                    continue;
                }
                if (!sc2::DoesFileExist(NewBot.RootPath + NewBot.FileName))
                {
                    std::cerr << "Unable to parse bot " << NewBot.BotName << std::endl;
                    std::cerr << "Is the path " << NewBot.RootPath << "correct?" << std::endl;
                    continue;
                }
                if (val.HasMember("Args") && val["Args"].IsString())
                {
                    NewBot.Args = val["Arg"].GetString();
                }
            }
            else
            {
                if (val.HasMember("Difficulty") && val["Difficulty"].IsString())
                {
                    NewBot.Difficulty = GetDifficultyFromString(val["Difficulty"].GetString());
                }
            }
            if (EnablePlayerIds)
            {
                NewBot.PlayerId = PlayerIds->GetValue(NewBot.BotName);
                if (NewBot.PlayerId.empty())
                {
                    NewBot.PlayerId = GerneratePlayerId(PLAYER_ID_LENGTH);
                    PlayerIds->AddValue(NewBot.BotName, NewBot.PlayerId);
                    PlayerIds->WriteConfig();
                }
            }
            botConfigs.insert(std::make_pair(std::string(NewBot.BotName), NewBot));

        }
    }

    return botConfigs;
}

std::unique_ptr<std::vector<std::string>> LadderBotsLoader::GetMaps() {
    std::unique_ptr<std::vector<std::string>> maps (new std::vector<std::string>);
    // todo error handling
    if (doc.HasMember("Maps") && doc["Maps"].IsArray())
    {
        const rapidjson::Value & Maps = doc["Maps"];
        for (auto itr = Maps.Begin(); itr != Maps.End(); ++itr)
        {
            maps->push_back(itr->GetString());
        }
    }
    return maps;
}
