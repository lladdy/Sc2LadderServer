#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <fstream>      
#include "LadderConfig.h"

LadderConfig::LadderConfig()
{
}

bool LadderConfig::FromFile(std::string filename) {
{
	std::ifstream ifs(filename, std::ifstream::binary);
	if (!ifs)
	{
		return false;
	}
	

	std::string line;
	while (std::getline(ifs, line))
	{
		std::istringstream is_line(line);
		std::string key;
		if (std::getline(is_line, key, '='))
		{
			std::string value;
			if (std::getline(is_line, value))
			{
				TrimString(key);
				TrimString(value);
				options.insert(std::make_pair(key, value));
			}
		}
	}
	return true;
}

bool LadderConfig::ToFile(std::string filename)
{
	std::ofstream outfile(filename, std::ofstream::binary);
	for (const auto& setting : options)
	{
		outfile << setting.first << "=" << setting.second << std::endl;
	}
	outfile.close();
	return true;
}

std::string LadderConfig::GetValue(std::string RequestedValue)
{
	auto search = options.find(RequestedValue);
	if (search != options.end())
	{
		return search->second;
	}
	return std::string();
}

void LadderConfig::TrimString(std::string &Str)
{
	size_t p = Str.find_first_not_of(" \t");
	Str.erase(0, p);
	p = Str.find_last_not_of(" \t\r\n");
	if (std::string::npos != p)
	{
		Str.erase(p + 1);
	}
}

void LadderConfig::AddValue(const std::string &Index, const std::string &Value)
{
	options.insert(std::make_pair(Index, Value));
}
