#pragma once

class LadderConfig
{
public:
    LadderConfig();
    bool FromFile(std::string filename);
	bool ToFile(std::string filename);
    std::string GetValue(std::string RequestedValue);
	void AddValue(const std::string &Index, const std::string &Value);
private:
    static void TrimString(std::string &Str);
//    const std::string ConfigFileLocation;
    std::map<std::string, std::string> options;


};
