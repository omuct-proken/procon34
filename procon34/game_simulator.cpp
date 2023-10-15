#include "game_instructions.hpp"
#include "game_simulator.hpp"

namespace Procon34 {

	TurnInstruction GameSimulator::initialize(BoxPtr<GameState> game) {
		TurnInstruction result(game, game->whosTurn());
		m_game = game;

		if (m_game->whosTurn() == PlayerColor::Red) {
			result = m_strategy1(m_game);
		}

		if (m_game->whosTurn() == PlayerColor::Blue) {
			result = m_strategy2(m_game);
		}

		m_game->makeMove(m_game->whosTurn(), result);

		return result;
	}

	TurnInstruction GameSimulator::advanceTurn(TurnInstruction turn) {
		TurnInstruction result(m_game, m_game->whosTurn());

		if (m_game->whosTurn() == PlayerColor::Red) {
			result = m_strategy1(turn.getGameState());
		}

		if (m_game->whosTurn() == PlayerColor::Blue) {
			result = m_strategy2(turn.getGameState());
		}

		m_game->makeMove(m_game->whosTurn(), result);

		return result;
	}

	// AgentMove を受け取ってそこから必要なデータ (type, direction) だけ取り出す
	AgentInstructionRecord GameSimulator::getAgentInstructionRecord(AgentMove agent) {

		AgentInstructionRecord result;

		if (agent.isMove()) {
			result.type = U"Move";
			result.direction = agent.asMove().dir;
		}

		else if (agent.isConstruct()) {
			result.type = U"Construct";
			result.direction = agent.asConstruct().dir;
		}

		else if (agent.isDestroy()) {
			result.type = U"Destroy";
			result.direction = agent.asDestroy().dir;
		}

		else {
			result.type = U"Stay";
			result.direction = unspecified;
		}

		return result;
	}

	// Agentinstructionrecord を呼び出してそれを配列 (TurnInstructionRecord) にして返す
	TurnInstructionRecord GameSimulator::getTurnInstructionRecord(TurnInstruction turn) {
		TurnInstructionRecord result;

		for (int i = 0; i < turn.size(); ++i) {
			result.addAgentInstructionRecord(getAgentInstructionRecord(turn[i]));
		}

		return result;
	}

	void GameSimulator::startSimulating() {
		m_record.initialState = *(m_game->getInitialState());

		// Turn 1
		TurnInstruction previous = initialize(m_game);
		m_record.addTurnInstructionRecord(getTurnInstructionRecord(previous));

		// Turn 2 ~
		while (!m_game->isOver()) {
			TurnInstruction current = advanceTurn(previous);

			m_record.addTurnInstructionRecord(getTurnInstructionRecord(current));

			previous = current;
		}
	}

	MatchRecord GameSimulator::getMatchRecord() {
		if (m_record.turns.isEmpty()) startSimulating();

		return m_record;
	}
}
