#pragma once
#include "stdafx.h"
#include "game_util.hpp"


namespace Procon34 {
	Optional<JSON> getMatchesList();
	Optional<JSON> getMatchesState(int64 id);
	void postAgentActions(int64 id, JSON json);


	struct MatchOverview {
		int64 id;
		int32 turns;
		int32 turnSeconds;
		String opponent;
		int32 bonusWall;
		int32 bonusTerritory;
		int32 bonusCastle;
		bool first;
		int32 boardWidth;
		int32 boardHeight;
		int32 boardMason;
		Array<BoardPos> myAgentPos;
		Array<BoardPos> opponentAgentPos;

		// [GET] /matches
		// で出てきた JSON を x として、
		// 配列 x[U"matches"] の要素を val に与える。
		static Optional<MatchOverview> FromJson(JSON val);
	};

}
