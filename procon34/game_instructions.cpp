#include "game_instructions.hpp"
#include "game_state.hpp"


namespace Procon34 {


	TurnInstruction::TurnInstruction(BoxPtr<const GameState> game, PlayerColor color)
		: m_game(game)
		, m_color(color)
	{
		m_data = game->getAgents(color).map([](Agent agent) { return AgentMove::GetStay(agent); });
	}

	void TurnInstruction::insert(AgentMove data) {
		auto agent = data.getAgent();
		if (!agent.has_value()) {
			throw Error(U"error at TurnInstruction::insert (data was null)");
		}
		if (m_color != agent->marker.color) {
			throw Error(U"error at TurnInstruction::insert (wrong color)");
		}
		auto idx = agent->marker.index;
		if (idx < 0) {
			throw Error(U"error at TurnInstruction::insert (idx < 0, agentNum = , idx = {} )"_fmt(idx));
		}
		if ((int32)m_data.size() <= idx) {
			throw Error(U"error at TurnInstruction::insert (agentNum <= idx, agentNum = , idx = {} )"_fmt(m_data.size(), idx));
		}
		m_data[idx] = data;
	}

	// 行動計画更新API の Request Body ([POST] /matches/{id})
	JSON TurnInstruction::toJson() const {
		JSON result;

		// 行動を計画しているターンを入れる
		result[U"turn"] = m_game->getTurnIndex() + 1;

		for (int idx = 0; idx < m_data.size(); idx++) { // 適当な for 文
			auto agent = m_data[idx];
			JSON action;

			if (agent.isStay()) {
				action[U"type"] = 0; 
				action[U"dir"] = 0; // 無方向
				result[U"actions"].push_back(action);
			}

			else if (agent.isMove()) {
				action[U"type"] = 1;
				action[U"dir"] = (11 - agent.asMove().dir.value()) % 8 + 1; 
				result[U"actions"].push_back(action);
			}

			else if (agent.isConstruct()) {
				action[U"type"] = 2;
				action[U"dir"] = (11 - agent.asConstruct().dir.value()) % 8 + 1;
				result[U"actions"].push_back(action);
			}

			else if (agent.isDestroy()) {
				action[U"type"] = 3;
				action[U"dir"] = (11 - agent.asDestroy().dir.value()) % 8 + 1;
				result[U"actions"].push_back(action);
			}
		}
		return result;
	}
}
