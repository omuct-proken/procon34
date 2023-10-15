#pragma once
#include "stdafx.h"
#include "game_util.hpp"
#include "game_agent.hpp"


namespace Procon34 {

	struct TurnInstruction {
	public:
		BoxPtr<const class GameState> m_game;
		PlayerColor m_color;
		Array<AgentMove> m_data;


		TurnInstruction(BoxPtr<const class GameState> game, PlayerColor color);

		// 職人 1 人の移動を設定する
		void insert(AgentMove data);

		BoxPtr<const class GameState> getGameState() const { return m_game; }

		PlayerColor whosTurn() const { return m_color; }

		const AgentMove& operator[](size_t i) const { return m_data.at(i); }

		size_t size() const { return m_data.size(); }

		// 行動計画更新API の Request Body ([POST] /matches/{id})
		JSON toJson() const;
	};

}
