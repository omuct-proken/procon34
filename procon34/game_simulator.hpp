#pragma once
#include "game_instructions.hpp"
#include "game_state.hpp"

namespace Procon34 {

	struct AgentInstructionRecord {
		String type;
		Optional<MoveDirection> direction;

		JSON toJson() const {
			JSON res;
			res[U"type"] = type;
			if (direction.has_value()) {
				res[U"direction"] = direction->value();
			}
			return res;
		}

		static AgentInstructionRecord FromJson(JSON json) {
			AgentInstructionRecord res;
			res.type = json[U"type"].getString();
			if (json.hasElement(U"direction")) res.direction = MoveDirection(json[U"direction"].get<int32>());
			return res;
		}
	};

	struct TurnInstructionRecord {
		Array<AgentInstructionRecord> instructions;

		void addAgentInstructionRecord(AgentInstructionRecord record) {
			instructions.push_back(record);
		}

		JSON toJson() const {
			JSON res;
			for (int32 i = 0; i < (int32)instructions.size(); i++) {
				res[i] = instructions[i].toJson();
			}
			return res;
		}

		static TurnInstructionRecord FromJson(JSON json) {
			TurnInstructionRecord res;
			for (int32 i = 0; i < (int32)json.size(); i++) {
				res.instructions.push_back(AgentInstructionRecord::FromJson(json[i]));
			}
			return res;
		}
	};

	struct MatchRecord {
		GameInitialState initialState;
		Array<TurnInstructionRecord> turns;

		void addTurnInstructionRecord(TurnInstructionRecord turn) {
			turns.push_back(turn);
		}

		JSON toJson() const {
			JSON res;
			res[U"initialState"] = initialState.toJson();
			JSON turnsJson;
			for (int32 i = 0; i < (int32)turns.size(); i++) {
				turnsJson[i] = turns[i].toJson();
			}
			res[U"turns"] = turnsJson;
			return res;
		}

		static MatchRecord FromJson(JSON json) {
			MatchRecord res;
			res.initialState = GameInitialState::FromJson(json[U"initialState"]);
			JSON turnsJson = json[U"turns"];
			for (int32 i = 0; i < (int32)json.size(); i++) {
				res.turns.push_back(TurnInstructionRecord::FromJson(turnsJson[i]));
			}
			return res;
		}
	};

	// 盤面の状態から指示を作る
	inline TurnInstruction strategy(BoxPtr<GameState> state) {
		const PlayerColor turn = state->whosTurn();
		const Array<Agent> agents = state->getAgents(turn);

		// --------------------------------
		// GameState を受け取ってする処理を書く
		// --------------------------------

		TurnInstruction result(state, turn);

		return result;
	}

	class GameSimulator {
	private:
		BoxPtr<GameState> m_game;
		std::function<TurnInstruction(BoxPtr<const GameState>)> m_strategy1;
		std::function<TurnInstruction(BoxPtr<const GameState>)> m_strategy2;
		MatchRecord m_record;
	public:
		GameSimulator(BoxPtr<GameState> game,
					   std::function<TurnInstruction(BoxPtr<const GameState>)> strategy1,
					   std::function<TurnInstruction(BoxPtr<const GameState>)> strategy2
		) : m_game(game), m_strategy1(strategy1), m_strategy2(strategy2) {}

		// 試合の途中から始めるときに使う
		TurnInstruction initialize(BoxPtr<GameState> game);

		TurnInstruction advanceTurn(TurnInstruction turn);

		AgentInstructionRecord getAgentInstructionRecord(AgentMove agent);

		TurnInstructionRecord getTurnInstructionRecord(TurnInstruction turn);

		void startSimulating();

		MatchRecord getMatchRecord();

	};

}
