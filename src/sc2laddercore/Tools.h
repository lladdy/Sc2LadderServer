#pragma once

#include <string>
#include "Types.h"

// ToolsCommon.cpp

std::vector<std::string> SplitStringByCharacter(const std::string& string, const char splitter);

bool FileExistsInExecutableDirectory(const std::string& filename);

bool FileExistsInWorkingDirectory(const std::string& filename);

bool FileExists(const std::string& filename);

bool CanFindFileInEnvironment(const std::string& filename);

// Tools<Platform>.cpp

void StartBotProcess(const BotConfig &Agent, const std::string& CommandLine, unsigned long *ProcessId);

void SleepFor(int seconds);

void KillSc2Process(unsigned long pid);

bool MoveReplayFile(const char* lpExistingFileName, const char* lpNewFileName);

std::string GetExecutableFullFilename();

std::string GetExecutableDirectory();

std::string GetWorkingDirectory();

bool FileExistsInEnvironmentPath(const std::string& filename);