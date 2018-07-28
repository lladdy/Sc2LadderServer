#pragma once

#include <string>
class Bots{
public:
    Bots(std::string botConfigFile);
    void Load();
private:
    std::string botConfigFile;
};

class Bot{
public:
};
