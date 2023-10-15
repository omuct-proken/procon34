#pragma once
#include "../stdafx.h"
#include "grid_walking.hpp"
#include "grid_shortpath.hpp"

namespace Procon34 {

	class ConstructWallPath {
	public:

		ConstructWallPath(
			std::shared_ptr<GridWalking> gridWalking,
			Array<std::shared_ptr<WallPath>> wallPathInstances
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

		int32 getNumberOfWallPositions();

	private:
		std::shared_ptr<GridWalking> m_gridWalking;
		Array<std::shared_ptr<WallPath>> m_wallPathInstances;
	};

}

