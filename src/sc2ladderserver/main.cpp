

#include <string.h>

#include <Types.h>
#include <LadderManager.h>

int main(int argc, char** argv) {
    PrintThread{} << "LadderManager started." << std::endl;

    LadderManager LadderMan(argc, argv);
    if (LadderMan.LoadSetup()) {
        LadderMan.RunLadderManager();
    }

    PrintThread{} << "Finished." << std::endl;
}

// THE FOLLOWING CODE APPEARS TO BE FREE OF THE REPLAY ISSUE
//#include "ReplayIssueDebug.h"
//int main(int argc, char** argv)
//{

//	// START INSTANCES
//
//    std::cout << "Servers listening" << std::endl;
//	server1.Listen("5677", "100000", "100000", "5");
//	server2.Listen("5678", "100000", "100000", "5");
//
//
//    std::cout << "Starting SC2 Instances" << std::endl;
//	// Find game executable and run it.
//	sc2::ProcessSettings process_settings;
//	sc2::GameSettings game_settings;
//	sc2::ParseSettings(argc, argv, process_settings, game_settings);
//	uint64_t Bot1ProcessId = sc2::StartProcess(process_settings.process_path,
//		{ "-listen", "127.0.0.1",
//		"-port", "5679",
////		"-verbose",
//		"-displayMode", "0",
//		"-dataVersion", process_settings.data_version }
//	);
//	uint64_t Bot2ProcessId = sc2::StartProcess(process_settings.process_path,
//		{ "-listen", "127.0.0.1",
//		"-port", "5680",
////		"-verbose",
//		"-displayMode", "0",
//		"-dataVersion", process_settings.data_version }
//	);
//
//    std::cout << "Connecting clients" << std::endl;
//
//	// Connect to running sc2 process.
//	int connectionAttemptsClient1 = 0;
//	while (!client1.Connect("127.0.0.1", 5679, false))
//	{
//		connectionAttemptsClient1++;
//		sc2::SleepFor(1000);
//		if (connectionAttemptsClient1 > 60)
//		{
//			std::cout << "Failed to connect client 1. BotProcessID: " << Bot1ProcessId << std::endl;
//			return -1;
//		}
//	}
//	int connectionAttemptsClient2 = 0;
//	while (!client2.Connect("127.0.0.1", 5680, false))
//	{
//		connectionAttemptsClient2++;
//		sc2::SleepFor(1000);
//		if (connectionAttemptsClient2 > 60)
//		{
//			std::cout << "Failed to connect client 2. BotProcessID: " << Bot2ProcessId << std::endl;
//			return -1;
//		}
//	}
//
//    std::cout << "Create game request" << std::endl;
//	sc2::GameRequestPtr Create_game_request = CreateStartGameRequest();
//	client1.Send(Create_game_request.get());
//	SC2APIProtocol::Response* create_response = nullptr;
//	if (client1.Receive(create_response, 100000))
//	{
//		std::cout << "Recieved create game response " << create_response->data().DebugString() << std::endl;
//		if (ProcessResponse(create_response->create_game()))
//		{
//			std::cout << "Create game successful" << std::endl << std::endl;
//		}
//	}
//
//
//    std::cout << "Start bots" << std::endl;
//	unsigned long Bot1ThreadId = 0;
//	unsigned long Bot2ThreadId = 0;
//	bool lose1 = true;
//    bool lose2 = false;
//	auto bot1ProgramThread = std::async(&StartDebugBot1, &Bot1ThreadId);
//	auto bot2ProgramThread = std::async(&StartDebugBot2, &Bot2ThreadId);
//	sc2::SleepFor(500);
//	sc2::SleepFor(500);
//
//
//	// RUN GAME
//
//    std::cout << "Game update" << std::endl;
//	std::string db1 = "DB1";
//    std::string db2 = "DB2";
//	auto bot1UpdateThread = std::async(&GameUpdate, &client1, &server1, &db1);
//	auto bot2UpdateThread = std::async(&GameUpdate, &client2, &server2, &db2);
//	sc2::SleepFor(1000);
//
//	bool GameRunning = true;
//	//sc2::ProtoInterface proto_1;
//	while (GameRunning)
//	{
//
////        std::cout << "Game running" << std::endl;
//		auto update1status = bot1UpdateThread.wait_for(std::chrono::milliseconds(1));
//		auto update2status = bot2UpdateThread.wait_for(std::chrono::milliseconds(0));
//		auto thread1Status = bot1ProgramThread.wait_for(std::chrono::milliseconds(0));
//		auto thread2Status = bot2ProgramThread.wait_for(std::chrono::milliseconds(0));
//		if (update1status == std::future_status::ready)
//		{
//			GameRunning = false;
//			break;
//		}
//		if (update2status == std::future_status::ready)
//		{
//			GameRunning = false;
//			break;
//		}
//		if (thread1Status == std::future_status::ready)
//		{
//			GameRunning = false;
//		}
//		if (thread2Status == std::future_status::ready)
//		{
//			GameRunning = false;
//		}
//
//	}
//
//    std::cout << "Saving replay" << std::endl;
//	auto now = std::chrono::system_clock::now();
//	std::string replayName = std::to_string(std::chrono::system_clock::to_time_t (now))
//	        + "_DebugBot1VsDebugBot2.SC2Replay";
//	SaveReplay(&client1,replayName);
//
//    sc2::SleepFor(1000);
//    if (!SendDataToConnection(&client1, CreateLeaveGameRequest().get()))
//    {
//        std::cout << "CreateLeaveGameRequest failed for Client 1." << std::endl;
//    }
//    sc2::SleepFor(1000);
//    if (!SendDataToConnection(&client2, CreateLeaveGameRequest().get()))
//    {
//        std::cout << "CreateLeaveGameRequest failed for Client 2." << std::endl;
//    }
//
//
//    sc2::SleepFor(1000);
//    if (server1.HasRequest() && server1.connections_.size() > 0)
//    {
//        server1.SendRequest(client1.connection_);
//    }
//    sc2::SleepFor(1000);
//    if (server2.HasRequest() && server2.connections_.size() > 0)
//    {
//        server2.SendRequest(client1.connection_);
//    }
//
//    std::cout << "Killing Bot processes" << std::endl;
//    int ret = kill(Bot1ThreadId, SIGKILL);
//    if (ret < 0)
//    {
//        std::cout << std::string("Failed to send SIGKILL, error:") +
//                     strerror(errno) << std::endl;
//    }
//    ret = kill(Bot2ThreadId, SIGKILL);
//    if (ret < 0)
//    {
//        std::cout << std::string("Failed to send SIGKILL, error:") +
//                     strerror(errno) << std::endl;
//    }
}

