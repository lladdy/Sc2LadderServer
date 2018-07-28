#ifdef _WIN32

#include "Tools.h"
#include "LadderManager.h"
#include <Windows.h>
#include <fstream>


void StartBotProcess(const BotConfig &Agent, const std::string &CommandLine, unsigned long *ProcessId)
{
	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Executes the given command using CreateProcess() and WaitForSingleObject().
	// Returns FALSE if the command could not be executed or if the exit code could not be determined.

	SECURITY_ATTRIBUTES securityAttributes;
	securityAttributes.nLength = sizeof(securityAttributes);
	securityAttributes.lpSecurityDescriptor = NULL;
	securityAttributes.bInheritHandle = TRUE;
	std::string logFile = Agent.RootPath + "/stderr.log";
	HANDLE h = CreateFile(logFile.c_str(),
		FILE_APPEND_DATA,
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		&securityAttributes,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	PROCESS_INFORMATION processInformation;
	STARTUPINFO startupInfo;
	DWORD flags = NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE; //CREATE_NO_WINDOW <-- also possible, but we don't see easily if bot is still running.

	ZeroMemory(&processInformation, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
	startupInfo.cb = sizeof(STARTUPINFO);
	startupInfo.dwFlags |= STARTF_USESTDHANDLES;
	startupInfo.hStdInput = INVALID_HANDLE_VALUE;
	startupInfo.hStdError = h;
	startupInfo.hStdOutput = NULL;

	DWORD exitCode;
	LPSTR cmdLine = const_cast<char *>(CommandLine.c_str());
	// Create the process

	BOOL result = CreateProcess(NULL, cmdLine,
		NULL, NULL, TRUE,
		flags,
		NULL, Agent.RootPath.c_str(), &startupInfo, &processInformation);


	if (!result)
	{
		// Get the error from the system
		LPSTR lpMsgBuf;
		DWORD dw = GetLastError();
		size_t size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
		std::string message(lpMsgBuf, size);
		PrintThread{} << "Starting bot: " << Agent.BotName << " with command:" << std::endl << CommandLine << std::endl << "...failed" << std::endl << "Error " << dw << " : " << message << std::endl;
		// Free resources created by the system
		LocalFree(lpMsgBuf);
		// We failed.
		exitCode = -1;
	}
	else
	{
		PrintThread{} << "Starting bot: " << Agent.BotName << " with command:" << std::endl << CommandLine << std::endl << "...success!" << std::endl;
		// Successfully created the process.  Wait for it to finish.
		*ProcessId = processInformation.dwProcessId;
		WaitForSingleObject(processInformation.hProcess, INFINITE);

		// Get the exit code.
		result = GetExitCodeProcess(processInformation.hProcess, &exitCode);

		// Close the handles.
		CloseHandle(processInformation.hProcess);
		CloseHandle(processInformation.hThread);

		if (!result)
		{
			// Could not get exit code.
			exitCode = -1;
		}

		// We succeeded.
	}
	CloseHandle(h);
}

void StartExternalProcess(const std::string CommandLine)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	// set the size of the structures
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	LPSTR cmdLine = const_cast<char *>(CommandLine.c_str());
	// Create the process

	BOOL result = CreateProcess(NULL, cmdLine,
		NULL, NULL, TRUE,
		0,
		NULL, NULL, &si, &pi);

	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

void SleepFor(int seconds)
{
	Sleep(seconds * 1000);
}

void KillSc2Process(unsigned long pid)
{
	DWORD dwDesiredAccess = PROCESS_TERMINATE;
	BOOL  bInheritHandle = FALSE;
	HANDLE hProcess = OpenProcess(dwDesiredAccess, bInheritHandle, pid);
	if (hProcess == NULL)
		return;

	TerminateProcess(hProcess, 0);
	CloseHandle(hProcess);
}

bool MoveReplayFile(const char* lpExistingFileName, const char* lpNewFileName) {
	return MoveFile(lpExistingFileName, lpNewFileName);
}

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

bool FileExistsInEnvironmentPath(const std::string& filename)
{
	// 65535 - longest windows path var
	DWORD buffSize = 65535;
	std::string buffer;
	buffer.resize(buffSize);
	GetEnvironmentVariable("PATH", &buffer[0], buffSize);
	auto paths = SplitStringByCharacter(std::string(buffer), ';');
	for (auto&& s : paths) {
		if (std::ifstream(s + "\\" + filename).good())
			return true;
	}
	return false;
}

std::string GetExecutableFullFilename()
{
	char buf[MAX_PATH + 1];
	auto bytes = GetModuleFileName(NULL, buf, MAX_PATH + 1);
	if (bytes == 0)
		throw "Error: Could not retrieve executable file name.";
	return std::string(buf);
}

std::string GetExecutableDirectory()
{
	auto executableFile = GetExecutableFullFilename();
	auto found = executableFile.find_last_of('\\');
	if (found == std::string::npos)
		throw "Expected backslash in executableFile, but none was present.";
	// strip out the executable filename at the end
	return executableFile.substr(0, found);
}

std::string GetWorkingDirectory()
{
	char buf[MAX_PATH + 1];
	if (GetCurrentDirectory(MAX_PATH + 1, buf))
		return buf;
	else
		throw "Unable to get working directory: call to GetCurrentDirectory() returned nothing.";
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