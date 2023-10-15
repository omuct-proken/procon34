#pragma once
#include "construct_wall_path.hpp"

namespace Procon34 {

	ConstructWallPath::ConstructWallPath(
		std::shared_ptr<GridWalking> gridWalking,
		Array<std::shared_ptr<WallPath>> wallPathInstances
	)
		: m_gridWalking(gridWalking)
		, m_wallPathInstances(wallPathInstances)
	{
	}


	Array<ConstructWallPath::Answer> ConstructWallPath::solve(Agent agent, int32 maxTurn, Array<int64> turnProfit) {
		if (maxTurn < 0) throw Error(U"error at ConstructWallPath::solve : maxTurn < 0");
		if(turnProfit.size() < (size_t)(maxTurn + 1)) throw Error(U"error at ConstructWallPath::solve : turnProfit.size() < maxTurn + 1");

		auto gridWalkingAnswer = m_gridWalking->solve(agent, maxTurn);
		Array<Answer> result;

		for (auto wallPath : m_wallPathInstances) {

			Array<Array<BoardPos>> validAgentPos(wallPath->getNumberOfBaseWallPositions());
			for (auto node : wallPath->nodes) {
				if (gridWalkingAnswer.positionToListIndex[node.agentPos.asPoint()] >= 0) {
					validAgentPos[node.baseId].push_back(node.agentPos);
				}
			}

			Array<std::pair<int64, ShortenMove::Type>> resultBuffer;
			const int64 NEGINF = -1001001001001001001;
			std::pair<int64, ShortenMove::Type> bufferDefaultVal = { NEGINF, ShortenMove::Stay() };
			resultBuffer.assign(wallPath->getNumberOfBaseWallPositions(), bufferDefaultVal);


			// 壁の始点
			for (int32 from = 0; from < (int32)validAgentPos.size(); from++) if (validAgentPos[from].size() >= 1) {

				// 壁構築の経路問題の入力を構築
				//   壁の始点からノードを検索し、職人の位置を決定
				Array<WallPath::MovingState> starters;
				auto base = wallPath->basePos[from];
				for (auto agentPos : validAgentPos[from]) {
					int32 gridWalkingPosId = gridWalkingAnswer.positionToListIndex[agentPos.asPoint()];
					for (int32 t = 0; t <= maxTurn; t++) {
						if ((int32)gridWalkingAnswer.shortPath[t].size() <= gridWalkingPosId) continue;
						WallPath::MovingState tmp;
						tmp.turnCount = t;
						tmp.firstMove = ShortenMove::Encode(gridWalkingAnswer.shortPath[t][gridWalkingPosId].firstMove);
						tmp.offsetProfit = gridWalkingAnswer.shortPath[t][gridWalkingPosId].profit;
						tmp.nodeId = wallPath->getNodeId(agentPos, base);
						starters.push_back(tmp);
					}
				}

				// 壁構築の経路問題のアルゴリズムを実行
				starters = wallPath->solve(maxTurn, std::move(starters));

				// 出力にターン数のペナルティを追加して集約
				for (auto& st : starters) {
					int32 baseId = wallPath->nodes[st.nodeId].baseId;
					int64 profit = st.offsetProfit + turnProfit[st.turnCount];
					if (resultBuffer[baseId].first >= profit) continue;
					resultBuffer[baseId].first = profit;
					resultBuffer[baseId].second = st.firstMove;
				}

				// 出力に追記
				//     しながら resultBuffer をリセット
				for (int32 to = 0; to < (int32)resultBuffer.size(); to++) {
					if (resultBuffer[from].first > NEGINF) {
						if (wallPath->isReversed()) {
							result.push_back(Answer{
								.from = to,
								.to = from,
								.profit = resultBuffer[from].first,
								.firstMove = ShortenMove::Decode(resultBuffer[from].second, agent)
							});
						}
						else {
							result.push_back(Answer{
								.from = from,
								.to = to,
								.profit = resultBuffer[from].first,
								.firstMove = ShortenMove::Decode(resultBuffer[from].second, agent)
							});
						}
						resultBuffer[from] = bufferDefaultVal;
					}
				}
			}

		}

		return result;
	}

	int32 ConstructWallPath::getNumberOfWallPositions() {
		return m_wallPathInstances[0]->getNumberOfBaseWallPositions();
	}


}

