#pragma once
#include "../stdafx.h"
#include "../game_state.hpp"
#include "shorten_move.hpp"

namespace Procon34 {

	class DiagonalGraph {
	public:

		// 要らないぶんは private にしたいけど、突貫工事ゆえ全てにアクセスできるのが便利なので

		// input
		std::shared_ptr<const GameState> m_game;
		Grid<int64> m_territoryScore;

		// マスの角をたどるとき、上下に動く場合に陣地獲得の利得を寄与させる。
		// 下に動くと、左にあるマスのぶんだけが領域に加算されたと考える。上に動いたときはマイナスする。
		// これにより、左回りに陣地をつくったときの利得を推定する。
		Grid<int64> m_diffDown;

		Grid<int32> m_toBaseId;
		Grid<int64> m_profitToGoToBasePos;
		Array<Point> m_idToBasePos;

		struct WallAccess { int32 from; int32 to; int64 diff; };
		Grid<int32> m_wallCandidateId;
		Array<BoardPos> m_wallCandidatePos;
		Array<Array<WallAccess>> m_wallAccess;

	public:

		DiagonalGraph(
			std::shared_ptr<const GameState> game,
			Grid<int64> territoryScore
		);

		int32 numberOfBases() const;

	};

}

