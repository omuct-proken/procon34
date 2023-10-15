#pragma once
#include "../stdafx.h"
#include "../game_state.hpp"

namespace Procon34 {

	struct GridWalking {

		std::shared_ptr<const GameState> game;
		Array<Grid<int64>> visitingProfit;
		int32 capableTurnCount;

		Size boardSize;

		GridWalking(
			std::shared_ptr<const GameState> _game,
			Array<Grid<int64>> _visitingProfit
		);

		// 見込まれる利得と、そのための最初の行動
		struct ShortPathAnswer {
			AgentMove firstMove;
			int64 profit;
		};

		struct Answer {
			int32 maxTurnCount; // 最大ターン数
			Array<BoardPos> posistions; // 登録された座標
			Grid<int32> positionToListIndex; // 無効なマスなら 0 未満
			Array<Array<ShortPathAnswer>> shortPath; // [turn][positions id]
		};

		Answer solve(Agent agent, int32 maxTurnCount);

	};

}

