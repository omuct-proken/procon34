#pragma once
#include "../stdafx.h"
#include "grid_walking.hpp"
#include "diag_graph.hpp"
#include "grid_shortpath_2.hpp"

namespace Procon34 {

	class ConstructWallPath2 {
	public:

		ConstructWallPath2(
			std::shared_ptr<DiagonalGraph> diagGraph,
			std::shared_ptr<GridWalking> gridWalking,
			Array<std::shared_ptr<WallPath2>> wallPathInstances
		);

		struct Answer {
			int32 from;
			int32 to;
			int64 profit;
			AgentMove firstMove;
		};

		// turnProfit[k] : k ターンかかるときの追加利得（ k について単調減少を想定）
		// 
		// from から to まで壁を作るときの最大利得と最初の操作、をたくさん返す。
		Array<Answer> solve(Agent agent, int32 maxTurn, Array<int64> turnProfit);

		int32 getNumberOfBases();

	private:
		std::shared_ptr<DiagonalGraph> m_diagGraph;
		std::shared_ptr<GridWalking> m_gridWalking;
		Array<std::shared_ptr<WallPath2>> m_wallPathInstances;
	};

}

