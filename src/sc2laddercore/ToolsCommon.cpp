#ifdef _WIN32

#include "Tools.h"

#include <fstream>
#include <sstream>

std::vector<std::string> SplitStringByCharacter(const std::string& string, const char splitter)
{
	std::string segment;
	std::vector<std::string> segments;

	std::stringstream stream(string);
	// split by designated splitter character
	while (getline(stream, segment, splitter))
		segments.push_back(segment);

	return segments;
}

bool FileExistsInExecutableDirectory(const std::string& filename)
{
	return std::ifstream(GetExecutableDirectory() + "\\" + filename).good();
}

bool FileExistsInWorkingDirectory(const std::string& filename)
{
	return std::ifstream(GetWorkingDirectory() + "\\" + filename).good();
}

bool FileExists(const std::string& filename)
{
	return std::ifstream(filename).good();
}

bool CanFindFileInEnvironment(const std::string& filename)
{
	// If filename contains directories, only check it against
	// the an absolute reference or the working directory.
	// Only straight commands such as "Bot.exe" (i.e. no directory)
	// will successfully run a program found in the executable's
	// directory or the env path
	return filename.find('\\') != std::string::npos
		|| filename.find('/') != std::string::npos
		? FileExists(filename)
		|| FileExistsInWorkingDirectory(filename)
		: FileExists(filename)
		|| FileExistsInExecutableDirectory(filename)
		|| FileExistsInWorkingDirectory(filename)
		|| FileExistsInEnvironmentPath(filename);
}

#endif