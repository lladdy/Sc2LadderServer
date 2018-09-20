#include <iostream>
#include "sc2api/sc2_api.h"
#include "sc2api/sc2_args.h"
#include "sc2api/sc2_common.h"
#include "sc2lib/sc2_lib.h"
#include "sc2utils/sc2_manage_process.h"
#include "sc2utils/sc2_arg_parser.h"

#include "LadderInterface.h"

#include <random>

enum class DebugMode {
	DoNothing,
	Lose,
	RandomMovement,
	RandomMovementThenLose
};

class DebugBot : public sc2::Agent {
public:
	DebugBot(CommandlineOptions Options) {
		std::cout << "DebugMode is: " << Options.DebugMode << std::endl;
		this->debugMode = ParseDebugModeString(Options.DebugMode);
	}
	virtual void OnGameStart() final {
		const sc2::ObservationInterface* observation = Observation();
		std::cout << "I am player number " << observation->GetPlayerID() << std::endl;
	};

	virtual void OnStep() final {
		stepNumber++;
		switch (this->debugMode)
		{
			case DebugMode::DoNothing: {
				// Do Nothing
			} break;
			case DebugMode::Lose: {
				this->KillSelf();
			} break;
			case DebugMode::RandomMovement: {
				this->RandomMovement();
			} break;
			case DebugMode::RandomMovementThenLose: {
				if (this->stepNumber < 1000) { // 7000 steps should be over 5 minutes in game
					this->RandomMovement();
				}
				else {
					this->KillSelf();
				}
			} break;
		}
	};

private:
	DebugMode debugMode = DebugMode::DoNothing;
	long stepNumber = 0;

	DebugMode ParseDebugModeString(std::string debugMode) {
		if (debugMode == "DoNothing") {
			return DebugMode::DoNothing;
		}
		else if (debugMode == "Lose") {
			return DebugMode::Lose;
		}
		else if (debugMode == "RandomMovement") {
			return DebugMode::RandomMovement;
		}
		else if (debugMode == "RandomMovementThenLose") {
			return DebugMode::RandomMovementThenLose;
		}
		else {
			std::cout << "Unrecognized debug mode. Using DoNothing." << std::endl;
			return DebugMode::DoNothing;
		}
	}

	void KillSelf() {
		const sc2::ObservationInterface* observation = Observation();
		sc2::ActionInterface* action = Actions();
		const sc2::Units units = observation->GetUnits(sc2::Unit::Alliance::Self);
		for (const auto& commandCenter : units) {
			if (static_cast<sc2::UNIT_TYPEID>(commandCenter->unit_type) == sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER)
			{
				for (const auto& scv : units) {
					if (static_cast<sc2::UNIT_TYPEID>(scv->unit_type) == sc2::UNIT_TYPEID::TERRAN_SCV) {
						action->UnitCommand(scv, sc2::ABILITY_ID::ATTACK, commandCenter);
					}
				}
			}
		}
	}

	void RandomMovement() {
		const sc2::ObservationInterface* observation = Observation();
		sc2::ActionInterface* action = Actions();
		const sc2::Units units = observation->GetUnits(sc2::Unit::Alliance::Self);
		sc2::Point2D target_pos;
		for (const auto& unit : units) {
			if (static_cast<sc2::UNIT_TYPEID>(unit->unit_type) == sc2::UNIT_TYPEID::TERRAN_SCV
				&& TryFindRandomPathableLocation(observation, unit, target_pos)) {
				Actions()->UnitCommand(unit, sc2::ABILITY_ID::SMART, target_pos);
			}
		}
	}

	// Taken from s2client-api bot_examples.cc:
	bool TryFindRandomPathableLocation(const sc2::ObservationInterface* observation, const sc2::Unit* unit, sc2::Point2D& target_pos) {
		auto game_info_ = observation->GetGameInfo();
		// First, find a random point inside the playable area of the map.
		float playable_w = game_info_.playable_max.x - game_info_.playable_min.x;
		float playable_h = game_info_.playable_max.y - game_info_.playable_min.y;

		// The case where game_info_ does not provide a valid answer
		if (playable_w == 0 || playable_h == 0) {
			playable_w = 236;
			playable_h = 228;
		}

		target_pos.x = playable_w * sc2::GetRandomFraction() + game_info_.playable_min.x;
		target_pos.y = playable_h * sc2::GetRandomFraction() + game_info_.playable_min.y;

		// Now send a pathing query from the unit to that point. Can also query from point to point,
		// but using a unit tag wherever possible will be more accurate.
		// Note: This query must communicate with the game to get a result which affects performance.
		// Ideally batch up the queries (using PathingDistanceBatched) and do many at once.
		float distance = Query()->PathingDistance(unit, target_pos);

		return distance > 0.1f;
	}
};

int main(int argc, char* argv[])
{
	// Parse command line options
	CommandlineOptions Options;
	ParseArguments(argc, argv, Options);

	// Run the bot
	RunBot(Options, new DebugBot(Options), sc2::Race::Terran);
}