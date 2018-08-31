#include "sc2lib/sc2_lib.h"
#include "sc2api/sc2_api.h"
#include "sc2api/sc2_interfaces.h"
#include "sc2api/sc2_score.h"
#include "sc2api/sc2_map_info.h"
#include "sc2utils/sc2_manage_process.h"
#include "sc2api/sc2_game_settings.h"
#include "sc2api/sc2_proto_interface.h"
#include "sc2api/sc2_interfaces.h"
#include "sc2api/sc2_proto_to_pods.h"
#include "s2clientprotocol/sc2api.pb.h"
#include "sc2api/sc2_server.h"
#include "sc2api/sc2_connection.h"
#include "sc2api/sc2_args.h"
#include "sc2api/sc2_client.h"
#include "sc2api/sc2_proto_to_pods.h"
#include "civetweb.h"

#include <fstream>
#include <iostream>
#include <future>
#include <thread>
#include <chrono>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

enum ExitCase
{
	InProgress,
	GameEnd,
	ClientRequestExit,
	ClientTimeout,
	GameTimeout
};

static std::string GetExitCaseString(ExitCase ExitCaseIn)
{
	switch (ExitCaseIn)
	{
	case ExitCase::ClientRequestExit:
		return "ClientRequestExit";
	case ExitCase::ClientTimeout:
		return "ClientTimeout";
	case ExitCase::GameEnd:
		return "GameEnd";
	case ExitCase::GameTimeout:
		return "GameTimeout";
	case ExitCase::InProgress:
		return "InProgress";
	}
	return "Error";
}

ExitCase GameUpdate(sc2::Connection *client, sc2::Server *server, const std::string *botName)
{
	//    std::cout << "Sending Join game request" << std::endl;
	//    sc2::GameRequestPtr Create_game_request = CreateJoinGameRequest();
	//    Client->Send(Create_game_request.get());
	ExitCase CurrentExitCase = ExitCase::InProgress;
	std::cout << "Starting proxy for " << *botName << std::endl;
	clock_t LastRequest = clock();
	std::map<SC2APIProtocol::Status, std::string> status;
	status[SC2APIProtocol::Status::launched] = "launched";
	status[SC2APIProtocol::Status::init_game] = "init_game";
	status[SC2APIProtocol::Status::in_game] = "in_game";
	status[SC2APIProtocol::Status::in_replay] = "in_replay";
	status[SC2APIProtocol::Status::ended] = "ended";
	status[SC2APIProtocol::Status::quit] = "quit";
	status[SC2APIProtocol::Status::unknown] = "unknown";
	SC2APIProtocol::Status OldStatus = SC2APIProtocol::Status::unknown;
	try
	{
		bool RequestFound = false;
		bool AlreadyWarned = false;
		while (CurrentExitCase == ExitCase::InProgress) {
			SC2APIProtocol::Status CurrentStatus;
			if (!client || !server)
			{
				std::cout << botName << " Null server or client returning ClientTimeout" << std::endl;
				return ExitCase::ClientTimeout;
			}
			if (client->connection_ == nullptr && RequestFound)
			{
				std::cout << "Client disconnect (" << *botName << ")" << std::endl;
				CurrentExitCase = ExitCase::ClientTimeout;
			}

			if (server->HasRequest())
			{
				const sc2::RequestData request = server->PeekRequest();
				if (request.second)
				{
					if (request.second->has_quit()) //Really paranoid here...
					{
						// Intercept leave game and quit requests, we want to keep game alive to save replays
						CurrentExitCase = ExitCase::ClientRequestExit;
						break;
					}
					else if (request.second->has_debug() && !AlreadyWarned)
					{
						std::cout << *botName << " IS USING DEBUG INTERFACE.  POSSIBLE CHEAT Please tell them not to" << std::endl;
						AlreadyWarned = true;
					}
				}
				if (client->connection_ != nullptr)
				{
					server->SendRequest(client->connection_);

				}

				// Block for sc2's response then queue it.
				SC2APIProtocol::Response* response = nullptr;
				client->Receive(response, 100000);
				if (response != nullptr)
				{
					CurrentStatus = response->status();
					if (OldStatus != CurrentStatus)
					{
						std::cout << "New status of " << *botName << ": " << status.at(CurrentStatus) << std::endl;
						OldStatus = CurrentStatus;
					}
					if (CurrentStatus > SC2APIProtocol::Status::in_replay)
					{
						CurrentExitCase = ExitCase::GameEnd;
					}
					if (response->has_observation())
					{
						const SC2APIProtocol::ResponseObservation LastObservation = response->observation();
						const SC2APIProtocol::Observation& ActualObservation = LastObservation.observation();
						uint32_t currentGameLoop = ActualObservation.game_loop();
						if (currentGameLoop > 60480)
						{
							CurrentExitCase = ExitCase::GameTimeout;
						}

					}

				}

				// Send the response back to the client.
				if (server->connections_.size() > 0 && client->connection_ != NULL)
				{
					server->QueueResponse(client->connection_, response);
					server->SendResponse();
				}
				else
				{
					CurrentExitCase = ExitCase::ClientTimeout;
				}
				LastRequest = clock();

			}
			else
			{
				if ((LastRequest + (50 * CLOCKS_PER_SEC)) < clock())
				{
					std::cout << "Client timeout (" << *botName << ")" << std::endl;
					CurrentExitCase = ExitCase::ClientTimeout;
				}
			}

		}
		std::cout << *botName << " Exiting with " << GetExitCaseString(CurrentExitCase) << std::endl;
		return CurrentExitCase;
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return ExitCase::ClientTimeout;
	}
}

void StartBotProcessOnLinux(unsigned long *ProcessId, bool *lose)
{
	pid_t pID = fork();

	if (pID < 0)
	{
		std::cerr << std::string("Can't fork the bot process, error: ") +
			strerror(errno) << std::endl;
		return;
	}

	if (pID == 0) // child
	{
//		int ret = chdir(RootPath.c_str());
//		if (ret < 0) {
//			std::cerr << ": Can't change working directory to " + RootPath +
//				", error: " + strerror(errno) << std::endl;
//			exit(errno);
//		}
//
//		if (RedirectOutput(Agent, STDERR_FILENO, "stderr.log") < 0)
//			exit(errno);
//
//		if (Agent.Debug)
//		{
//			if (RedirectOutput(Agent, STDOUT_FILENO, "stdout.log") < 0)
//				exit(errno);
//		}
//		else
//			close(STDOUT_FILENO);
//
//		close(STDIN_FILENO);

//		std::vector<char*> unix_cmd;
//		std::istringstream stream(CommandLine);
//		std::istream_iterator<std::string> begin(stream), end;
//		std::vector<std::string> tokens(begin, end);
//		for (const auto& i : tokens)
//			unix_cmd.push_back(const_cast<char*>(i.c_str()));
        std::vector<char*> unix_cmd;
        if(*lose)
            unix_cmd.push_back(const_cast<char*>("-d RandomMovementThenLose"));

		// FIXME (alkurbatov): Unfortunately, the cmdline uses relative path.
		// This hack is needed because we have to change the working directory
		// before calling to exec.
		unix_cmd[0] = const_cast<char*>("DebugBot");

		unix_cmd.push_back(NULL);

		int ret = execv(unix_cmd.front(), &unix_cmd.front());

		if (ret < 0)
		{
			std::cerr << std::string(": Failed to execute, error: ") + strerror(errno) << std::endl;
			exit(errno);
		}

		exit(0);
	}

	// parent
	*ProcessId = pID;

	int exit_status = 0;
	int ret = waitpid(pID, &exit_status, 0);
	if (ret < 0) {
		std::cerr << std::string("Can't wait for the child process, error:") +
			strerror(errno) << std::endl;
	}
}

sc2::GameRequestPtr CreateStartGameRequest()
{
	sc2::ProtoInterface proto;
	sc2::GameRequestPtr request = proto.MakeRequest();

	SC2APIProtocol::RequestCreateGame* request_create_game = request->mutable_create_game();

	// DebugBot1
	SC2APIProtocol::PlayerSetup* playerSetup1 = request_create_game->add_player_setup();
	playerSetup1->set_type(SC2APIProtocol::PlayerType::Participant);
	playerSetup1->set_race(SC2APIProtocol::Race::Terran);
	playerSetup1->set_difficulty(SC2APIProtocol::Difficulty::Easy);

	// DebugBot2
	SC2APIProtocol::PlayerSetup* playerSetup2 = request_create_game->add_player_setup();
	playerSetup2->set_type(SC2APIProtocol::PlayerType::Participant);
	playerSetup2->set_race(SC2APIProtocol::Race::Terran);
	playerSetup2->set_difficulty(SC2APIProtocol::Difficulty::Easy);
	request_create_game->mutable_local_map()->set_map_path("./AcidPlantLE.SC2Map");

	request_create_game->set_realtime(false);
	return request;
}



bool ProcessResponse(const SC2APIProtocol::ResponseCreateGame& response)
{
	bool success = true;
	if (response.has_error()) {
		std::string errorCode = "Unknown";
		switch (response.error()) {
		case SC2APIProtocol::ResponseCreateGame::MissingMap: {
			errorCode = "Missing Map";
			break;
		}
		case SC2APIProtocol::ResponseCreateGame::InvalidMapPath: {
			errorCode = "Invalid Map Path";
			break;
		}
		case SC2APIProtocol::ResponseCreateGame::InvalidMapData: {
			errorCode = "Invalid Map Data";
			break;
		}
		case SC2APIProtocol::ResponseCreateGame::InvalidMapName: {
			errorCode = "Invalid Map Name";
			break;
		}
		case SC2APIProtocol::ResponseCreateGame::InvalidMapHandle: {
			errorCode = "Invalid Map Handle";
			break;
		}
		case SC2APIProtocol::ResponseCreateGame::MissingPlayerSetup: {
			errorCode = "Missing Player Setup";
			break;
		}
		case SC2APIProtocol::ResponseCreateGame::InvalidPlayerSetup: {
			errorCode = "Invalid Player Setup";
			break;
		}
		default: {
			break;
		}
		}

		std::cerr << "CreateGame request returned an error code: " << errorCode << std::endl;
		success = false;
	}

	if (response.has_error_details() && response.error_details().length() > 0) {
		std::cerr << "CreateGame request returned error details: " << response.error_details() << std::endl;
		success = false;
	}
	return success;

}

bool SendDataToConnection(sc2::Connection *Connection, const SC2APIProtocol::Request *request)
{
	if (Connection->connection_ != nullptr)
	{
		Connection->Send(request);
		return true;
	}
	return false;
}

bool SaveReplay(sc2::Connection *client, const std::string& path) {
	sc2::ProtoInterface proto;
	sc2::GameRequestPtr request = proto.MakeRequest();
	request->mutable_save_replay();
	SendDataToConnection(client, request.get());
	SC2APIProtocol::Response* replay_response = nullptr;
	if (!client->Receive(replay_response, 10000))
	{
		//		std::cout << "Failed to receive replay response" << std::endl;
		return false;
	}

	const SC2APIProtocol::ResponseSaveReplay& response_replay = replay_response->save_replay();

	if (response_replay.data().size() == 0) {
		return false;
	}

	std::ofstream file;
	file.open(path, std::fstream::binary);
	if (!file.is_open()) {
		return false;
	}

	file.write(&response_replay.data()[0], response_replay.data().size());
	return true;
}

sc2::Server server1;
sc2::Server server2;
sc2::Connection client1;
sc2::Connection client2;

int main(int argc, char** argv)
{
	// START INSTANCES

	server1.Listen("5677", "100000", "100000", "5");
	server2.Listen("5678", "100000", "100000", "5");
	// Find game executable and run it.
	sc2::ProcessSettings process_settings;
	sc2::GameSettings game_settings;
	sc2::ParseSettings(argc, argv, process_settings, game_settings);
	uint64_t Bot1ProcessId = sc2::StartProcess(process_settings.process_path,
		{ "-listen", "127.0.0.1",
		"-port", "5679",
		"-displayMode", "0",
		"-dataVersion", process_settings.data_version }
	);
	uint64_t Bot2ProcessId = sc2::StartProcess(process_settings.process_path,
		{ "-listen", "127.0.0.1",
		"-port", "5680",
		"-displayMode", "0",
		"-dataVersion", process_settings.data_version }
	);

	// Connect to running sc2 process.
	int connectionAttemptsClient1 = 0;
	while (!client1.Connect("127.0.0.1", 5679, false))
	{
		connectionAttemptsClient1++;
		sc2::SleepFor(1000);
		if (connectionAttemptsClient1 > 60)
		{
			std::cout << "Failed to connect client 1. BotProcessID: " << Bot1ProcessId << std::endl;
			return -1;
		}
	}
	int connectionAttemptsClient2 = 0;
	while (!client2.Connect("127.0.0.1", 5680, false))
	{
		connectionAttemptsClient2++;
		sc2::SleepFor(1000);
		if (connectionAttemptsClient2 > 60)
		{
			std::cout << "Failed to connect client 2. BotProcessID: " << Bot2ProcessId << std::endl;
			return -1;
		}
	}

	sc2::GameRequestPtr Create_game_request = CreateStartGameRequest();
	client1.Send(Create_game_request.get());
	SC2APIProtocol::Response* create_response = nullptr;
	if (client1.Receive(create_response, 100000))
	{
		std::cout << "Recieved create game response " << create_response->data().DebugString() << std::endl;
		if (ProcessResponse(create_response->create_game()))
		{
			std::cout << "Create game successful" << std::endl << std::endl;
		}
	}
	unsigned long Bot1ThreadId = 0;
	unsigned long Bot2ThreadId = 0;
	bool lose1 = true;
    bool lose2 = false;
	auto bot1ProgramThread = std::async(&StartBotProcessOnLinux, &Bot1ThreadId, &lose1);
	auto bot2ProgramThread = std::async(&StartBotProcessOnLinux, &Bot2ThreadId, &lose2);
	sc2::SleepFor(500);
	sc2::SleepFor(500);


	// RUN GAME

	std::string db1 = "DB1";

    std::string db2 = "DB2";
	auto bot1UpdateThread = std::async(&GameUpdate, &client1, &server1, &db1);
	auto bot2UpdateThread = std::async(&GameUpdate, &client2, &server2, &db2);
	sc2::SleepFor(1000);

	bool GameRunning = true;
	//sc2::ProtoInterface proto_1;
	while (GameRunning)
	{
		auto update1status = bot1UpdateThread.wait_for(std::chrono::seconds(0));
		auto update2status = bot2UpdateThread.wait_for(std::chrono::seconds(0));
		auto thread1Status = bot1ProgramThread.wait_for(std::chrono::seconds(0));
		auto thread2Status = bot2ProgramThread.wait_for(std::chrono::seconds(0));
		if (update1status == std::future_status::ready)
		{
			GameRunning = false;
			break;
		}
		if (update2status == std::future_status::ready)
		{
			GameRunning = false;
			break;
		}
		if (thread1Status == std::future_status::ready)
		{
			GameRunning = false;
		}
		if (thread2Status == std::future_status::ready)
		{
			GameRunning = false;
		}

	}

	SaveReplay(&client1, "DebugBot1VsDebugBot2.SC2Replay");
}

