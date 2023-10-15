#pragma once
#include "grid_shortpath.hpp"
#include "../game_state.hpp"
#include "../dsu_fast.hpp"

namespace Procon34 {


	// 隣接するマスに順に壁を置いた時の利得
	int64 WallPath::territoryScoreDiffCcw(BoardPos from, BoardPos to) {
		// 反時計回り
		//   < ----
		// |        ^
		// |        |
		// v        |
		//   ---- >
		if (from.r == to.r) {
			if (from.c < to.c) return 0; // 右 ： 変化なし
			return -territoryScore[to.asPoint()]; // 左 ： マスで陣地予定地が埋まる
		}
		if (from.r < to.r) {
			// 下 ： 陣地予定地が減る
			return -territoryScoreCum[BoardPos(to.r, to.c + 1).asPoint()];
		}
		// 上 ： 陣地予定地が増える
		return territoryScoreCum[BoardPos(to.r, to.c).asPoint()];
	}

	// 隣接するマスに順に壁を置いた時の利得
	int64 WallPath::territoryScoreDiffCw(BoardPos from, BoardPos to) {
		return territoryScoreDiffCcw(to, from);
	}

	WallPath::NodeId WallPath::getNodeId(BoardPos agentPos, BoardPos wallPos) {
		int32 base = baseId[wallPos.asPoint()];
		auto& ref = nodesIndexedByAgentPos[agentPos.asPoint()];
		auto iter = std::lower_bound(ref.begin(), ref.end(), std::make_pair(base, -1));
		if (iter == ref.end()) return -1;
		if (iter->first != base) return -1;
		return iter->second;
	}

	bool WallPath::isReversed() const {
		return m_asReversed;
	}

	WallPath::WallPath(
		std::shared_ptr<const GameState> _game,
		const Grid<int64>& _territoryScore,
		const Grid<int64>& _wallScore,
		Array<Point> _enabledDifference,
		bool asReversed
	)
		: game(_game)
		, territoryScore(_territoryScore)
		, wallScore(_wallScore)
		, enabledDifference(_enabledDifference)
		, territoryScoreCum()
		, m_asReversed(asReversed)
	{

		// 既存の壁の連結成分の計算

		auto board = game->getBoard();
		auto player = game->whosTurn();
		int32 height = board->getHeight();
		int32 width = board->getWidth();
		Size gridSize = board->getSize();
		assert(territoryScore.size() == gridSize);
		assert(wallScore.size() == gridSize);

		// 累積和
		territoryScoreCum.assign(width + 2, height, 0);
		for (int32 y = 0; y < height; y++) {
			for (int32 x = 0; x < width; x++) {
				territoryScoreCum[y][x + 1] = territoryScoreCum[y][x] + territoryScore[y][x];
			}
			territoryScoreCum[y][width + 1] = territoryScoreCum[y][width];
		}

		differenceFromBase.assign(gridSize, 0);
		baseId.assign(gridSize, -1);

		basePos.clear();
		{
			Grid<int> vis(gridSize, 0);

			for (int32 sy = 0; sy < height; sy++) for (int32 sx = 0; sx < width; sx++) {

				auto posS = BoardPos(sy, sx);
				if (baseId[posS.asPoint()] >= 0) continue; // 探索済み？
				if (board->getMass(posS).biome == MassBiome::Castle) continue; // 壁を置ける？

				int32 id = (int32)basePos.size();
				basePos.push_back(posS);
				baseId[posS.asPoint()] = id;

				if (board->getMass(posS).hasWallOf(player)) {

					// BFS で壁の連結成分を探索、 base から移動しながら利得を計算
					Array<BoardPos> que = { posS };
					for (size_t i = 0; i < que.size(); i++) {
						auto pos = que[i];
						for (int32 dirval = 0; dirval < 8; dirval++) {
							auto direction = MoveDirection(dirval);
							auto nxpos = pos.movedAlong(direction);
							if (!board->isOnBoard(nxpos)) continue; // フィールド内？
							if (baseId[nxpos.asPoint()] >= 0) continue; // 未探索？
							if (!board->getMass(nxpos).hasWallOf(player)) continue; // 壁がある？

							int64 difference = differenceFromBase[pos.asPoint()];
							difference += asReversed ? territoryScoreDiffCcw(pos, nxpos) : territoryScoreDiffCw(pos, nxpos);
							differenceFromBase[nxpos.asPoint()] = difference;
							baseId[nxpos.asPoint()] = baseId[pos.asPoint()];
							que.push_back(nxpos);
						}
					}
				}
			}
		}


		// ノードの集合の構築

		Grid<std::map<int32, NodeId>> nodeMapping(gridSize);
		int32 nextInsertedNodeId = 0;

		auto nodeInfoToId = [&](BoardPos agentPos, BoardPos wallPos) -> int32 {
			if (!board->isOnBoard(agentPos)) return -1;
			if (!board->isOnBoard(wallPos)) return -1;
			int32 wallPosId = baseId[wallPos.asPoint()];
			auto agentPosP = agentPos.asPoint();
			auto iter = nodeMapping[agentPosP].find(wallPosId);
			if (iter == nodeMapping[agentPosP].end()) return -1;
			return iter->second;
			};
		auto insertNodeId = [&](BoardPos agentPos, BoardPos wallPos) -> int32 {
			if (!board->isOnBoard(agentPos)) return -1;
			if (!board->isOnBoard(wallPos)) return -1;
			auto agentPosP = agentPos.asPoint();
			int32 wallPosId = baseId[wallPos.asPoint()];
			auto iter = nodeMapping[agentPosP].find(wallPosId);
			if (iter != nodeMapping[agentPosP].end()) return iter->second;
			nodeMapping[agentPosP][wallPosId] = nextInsertedNodeId;
			nextInsertedNodeId++;
			return nextInsertedNodeId - 1;
			};

		{

			for (int32 agentY = 0; agentY < height; agentY++) for (int32 agentX = 0; agentX < width; agentX++) {
				auto agentPos = BoardPos(agentY, agentX);
				if (board->getMass(agentPos).biome == MassBiome::Pond) continue; // 侵入可能？

				for (auto difference : enabledDifference) {
					auto wallPos = BoardPos(agentY + difference.y, agentX + difference.x);
					if (!board->isOnBoard(wallPos)) continue; // フィールド内？
					if (board->getMass(wallPos).biome == MassBiome::Castle) continue; // 建築可能？
					insertNodeId(agentPos, wallPos);
				}
			}

			nodes.resize(nextInsertedNodeId);
			for (int32 agentY = 0; agentY < height; agentY++) for (int32 agentX = 0; agentX < width; agentX++) {
				auto agentPos = BoardPos(agentY, agentX);
				for (auto node : nodeMapping[agentPos.asPoint()]) {
					nodes[node.second].agentPos = agentPos;
					nodes[node.second].wallPos = basePos[node.first];
					nodes[node.second].baseId = node.first;
					nodes[node.second].nodeId = node.second;
				}
			}
		}


		// ノードを検索できるようにする

		nodesIndexedByAgentPos = nodeMapping.map(
			[](const std::map<int32, NodeId>& x) -> Array<std::pair<int32, NodeId>> {
				return Array<std::pair<int32, NodeId>>(x.begin(), x.end());
			}
		);

		nodesIndexedByWallPos.resize(basePos.size());
		for (auto& v : nodes) {
			nodesIndexedByWallPos[baseId[v.wallPos.asPoint()]].push_back(v.nodeId);
		}



		// 遷移を計算

		for (auto node : nodes) {
			auto agentPos = node.agentPos;

			// 職人が移動する遷移の計算
			for (int32 moveDirectionVal = 0; moveDirectionVal < 8; moveDirectionVal++) {
				auto moveDirection = MoveDirection(moveDirectionVal);
				auto newAgentPos = agentPos.movedAlong(moveDirection);
				int32 nextNodeId = nodeInfoToId(newAgentPos, node.wallPos);
				if (nextNodeId < 0) continue;

				EdgeDesc tmp;
				tmp.cost = 0;

				if (newAgentPos.asPoint() != node.wallPos.asPoint()
					&& board->getMass(newAgentPos).hasWallOf(GameState::OpponentOf(player)) // 敵の壁があって、そのままでは通行不可
					) {
					if (!moveDirection.is4Direction()) continue;
					tmp.type = 2;
					tmp.from = node.nodeId;
					tmp.to = nextNodeId;
					tmp.turns = 2;
					tmp.cost = 0;
					tmp.direction = moveDirection;
				}
				else {
					tmp.type = 0;
					tmp.from = node.nodeId;
					tmp.to = nextNodeId;
					tmp.turns = 1;
					tmp.cost = 0;
					tmp.direction = moveDirection;
				}

				edges.push_back(tmp);
			}

			// 壁を設置する遷移の計算
			for (int32 wallDirectionVal = 0; wallDirectionVal < 8; wallDirectionVal += 2) {
				assert(MoveDirection(wallDirectionVal).is4Direction());

				auto wallPos = agentPos.movedAlong(MoveDirection(wallDirectionVal)); // これから建設するマス
				int32 wallPosNodeId = nodeInfoToId(agentPos, wallPos);
				if (wallPosNodeId < 0) continue; // 有効？
				if (board->getMass(wallPos).hasWallOf(player)) continue; // 未建設？

				bool existOpponentWall = board->getMass(wallPos).hasWallOf(GameState::OpponentOf(player));

				struct Jumping {
					int32 from;
					int32 to;
					int64 cost;
					MoveDirection direction;
				};
				Array<Jumping> immediateJumpList; // これから建設するマスに、隣接する壁
				Array<Jumping> previousWallList; // 直前の壁の位置としてありうるもの

				for (int32 adjDirectionVal = 0; adjDirectionVal < 8; adjDirectionVal++) {
					auto adjDirection = MoveDirection(adjDirectionVal);
					auto adjWallPos = wallPos.movedAlong(adjDirection); // これから建設するマス に隣接する座標
					int32 adjWallPosNodeId = nodeInfoToId(agentPos, adjWallPos);
					if (adjWallPosNodeId < 0) continue; // 有効？

					if (board->getMass(adjWallPos).hasWallOf(player)) {
						if (!immediateJumpList.includes_if([adjWallPosNodeId](Jumping j) -> bool { return j.to == adjWallPosNodeId; })) {
							int64 cost = asReversed ? territoryScoreDiffCcw(wallPos, adjWallPos) : territoryScoreDiffCw(wallPos, adjWallPos);
							immediateJumpList.push_back(
								Jumping{
									.from = wallPosNodeId,
									.to = adjWallPosNodeId,
									.cost = cost,
									.direction = adjDirection,
								}
							);
						}
					}
					{
						if (!previousWallList.includes_if([adjWallPosNodeId](Jumping j) -> bool { return j.from == adjWallPosNodeId; })) {
							int64 cost = asReversed ? territoryScoreDiffCcw(adjWallPos, wallPos) : territoryScoreDiffCw(adjWallPos, wallPos);
							cost += wallScore[wallPos.asPoint()];
							previousWallList.push_back(
								Jumping{
									.from = adjWallPosNodeId,
									.to = wallPosNodeId,
									.cost = cost,
									.direction = adjDirection.ccw45Deg(4)
								}
							);
						}
					}
				}

				for (auto previousWall : previousWallList) {
					edges.push_back(EdgeDesc{
						.from = previousWall.from,
						.to = previousWall.to,
						.type = existOpponentWall ? 3 : 1,
						.direction = MoveDirection(wallDirectionVal),
						.turns = existOpponentWall ? 2 : 1,
						.cost = previousWall.cost
					});

					for (auto immediateJump : immediateJumpList) {
						if (immediateJump.to == previousWall.from) continue;
						edges.push_back(EdgeDesc{
							.from = previousWall.from,
							.to = immediateJump.to,
							.type = existOpponentWall ? 3 : 1,
							.direction = MoveDirection(wallDirectionVal),
							.turns = existOpponentWall ? 2 : 1,
							.cost = previousWall.cost + immediateJump.cost
						});
					}
				}
			}
		}

		edges.sort_by([](const EdgeDesc& l, const EdgeDesc& r) { return l.from < r.from; });
		edgesSeparators.assign(nodes.size() + 1, 0);
		for (auto& e : edges) edgesSeparators[e.from + 1]++;
		for (size_t i = 0; i < nodes.size(); i++) edgesSeparators[i + 1] += edgesSeparators[i];

	}

	Array<Point> WallPath::EnabledDifferenceDefaultValue() {
		Array<Point> res;
		for (int32 dx = -2; dx <= 2; dx++) for (int32 dy = -2; dy <= 2; dy++) if (abs(dx) + abs(dy) <= 3) {
			res.push_back(Point(dx, dy));
		}
		return res;
	}

	int32 WallPath::getBaseOf(BoardPos pos) const { return baseId[pos.asPoint()]; }
	int32 WallPath::getNumberOfBaseWallPositions() const { return (int32)basePos.size(); }

	Array<AgentMove> WallPath::doOneCycle(Agent agent, int32 turnCount) {
		auto board = game->getBoard();
		Array<int32> usedNode(nodes.size());
		for (auto node : nodes) {
			if (board->getMass(node.agentPos).hasAgent() && board->getMass(node.agentPos).agent.value() != agent.marker) continue;
			if (board->getMass(node.wallPos).hasAgent() && board->getMass(node.wallPos).agent.value() != agent.marker) continue;
			usedNode[node.nodeId] = 1;
		}

		NodeId node = getNodeId(agent.pos, agent.pos);
		Array<Array<std::pair<int64, int32>>> dp(turnCount + 1, Array<std::pair<int64, int32>>(nodes.size(), std::make_pair(-1001001001001001001, -1)));
		dp[0][node] = { 0, -1 };
		for (int32 turn = 0; turn < turnCount; turn++) {
			for (int32 ei = 0; ei < (int32)edges.size(); ei++) {
				auto e = edges[ei];
				if (usedNode[e.from] == 0) continue;
				if (usedNode[e.to] == 0) continue;
				if (e.turns + turn <= turnCount) {
					dp[e.turns + turn][e.to] = std::max(dp[e.turns + turn][e.to], std::make_pair(dp[turn][e.from].first + e.cost, ei));
				}
			}
		}
		auto pos = node;
		Array<AgentMove> res;
		for (int32 turn = turnCount; turn > 0; turn--) {
			if (dp[turn][pos].second < 0) break;
			auto edge = edges[dp[turn][pos].second];
			switch (edge.type) {
				// type 0 : 職人が移動
				// type 1 : 壁を建設
				// type 2 : 壁を破壊して職人が移動
				// type 3 : 壁を破壊して壁を建設
			case 0:
				res.push_back(AgentMove::GetMove(agent, edge.direction));
				break;
			case 1:
				res.push_back(AgentMove::GetConstruct(agent, edge.direction));
				break;
			case 2:
				res.push_back(AgentMove::GetDestroy(agent, edge.direction));
				res.push_back(AgentMove::GetMove(agent, edge.direction));
				break;
			case 3:
				res.push_back(AgentMove::GetDestroy(agent, edge.direction));
				res.push_back(AgentMove::GetConstruct(agent, edge.direction));
				break;
			}
			pos = edge.from;
		}
		res.reverse();

		return res;
	}


	WallPath::CompressedByDistance WallPath::compressGraphsByDistance(int32 maxDistance, Array<std::pair<int32, int32>> starters) const {
		starters.sort_by([](auto l, auto r) { return l.second < r.second; });
		size_t starterPointer = 0;

		Array<int32> nodesMapping;
		Array<int32> distanceSeparator = { 0 };
		Array<EdgeDesc> newEdges;
		Array<int32> newEdgesSeparators;

		Array<int32> visited(nodes.size(), maxDistance + 1);
		Array<int32> nx1, nx2;
		for (int32 dist = 0; dist <= maxDistance; dist++) {
			while (starterPointer < starters.size() && starters[starterPointer].second <= dist) {
				int pos = starters[starterPointer].first;
				if (visited[pos] > dist) {
					nx1.push_back(pos);
					visited[pos] = dist;
				}
				starterPointer++;
			}

			auto nx0 = std::move(nx1);
			nx1 = std::move(nx2);
			nx2 = {};

			for (int32 v : nx0) if (visited[v] == dist) {
				nodesMapping.push_back(v);
				for (int32 ei = edgesSeparators[v]; ei < edgesSeparators[v + 1]; ei++) {
					auto& e = edges[ei];
					newEdges.push_back(e);
					int32 nxdist = e.turns + dist;
					if (nxdist >= visited[e.to]) continue;
					(e.turns == 2 ? nx2 : nx1).push_back(e.to);
					visited[e.to] = nxdist;
				}
			}

			newEdgesSeparators.push_back((int32)newEdges.size());


			distanceSeparator.push_back((int32)nodesMapping.size());
		}

		Array<int32> nodesInverseMapping(nodes.size());
		for (int32 i = 0; i < (int32)nodesMapping.size(); i++) {
			nodesInverseMapping[nodesMapping[i]] = i;
		}
		for (auto& e : newEdges) {
			e.from = nodesInverseMapping[e.from];
			e.to = nodesInverseMapping[e.to];
		}

		CompressedByDistance res;
		res.distanceSeparator = std::move(distanceSeparator);
		res.edges = std::move(newEdges);
		res.edgesSeparator = std::move(newEdgesSeparators);
		res.nodesMapping = std::move(nodesMapping);

		return res;
	}


	Array<WallPath::MovingState> WallPath::solve(int32 maxTurn, Array<MovingState> starters) {
		auto callCompressGraphsByDistance = [&]() -> WallPath::CompressedByDistance {
			Array<std::pair<int32, int32>> newStarters;
			for (auto& start : starters) {
				newStarters.push_back(std::make_pair(start.nodeId, start.turnCount));
			}
			return compressGraphsByDistance(maxTurn, std::move(newStarters));
		};
		auto compressedGraph = callCompressGraphsByDistance();

		Array<size_t> distanceArrayOffset(maxTurn + 2, 0);
		for (int32 d = 0; d <= maxTurn; d++) distanceArrayOffset[d + 1] = distanceArrayOffset[d] + compressedGraph.distanceSeparator[d + 1];

		Array<WallPath::MovingState> result(distanceArrayOffset.back());

		// 初期状態を result に反映
		{
			Array<int32> originalToCompressed(nodes.size());
			for (int32 i = 0; i < (int32)compressedGraph.nodesMapping.size(); i++) {
				originalToCompressed[compressedGraph.nodesMapping[i]] = i;
			}
			for (auto& starter : starters) {
				size_t resultArrayIndex = distanceArrayOffset[starter.turnCount] + originalToCompressed[starter.nodeId];
				result[resultArrayIndex] = starter;
			}
		}

		// 動的計画法で result 全体を計算

		for (int32 turnId = 0; turnId <= maxTurn; turnId++) {
			for (int32 nodeId = 0; nodeId < compressedGraph.distanceSeparator[turnId + 1]; nodeId++) {
				result[distanceArrayOffset[turnId] + nodeId].turnCount = turnId;
				result[distanceArrayOffset[turnId] + nodeId].nodeId = compressedGraph.nodesMapping[nodeId];
			}
			if (turnId != maxTurn) {
				for (int32 edgeId = 0; edgeId < compressedGraph.edgesSeparator[turnId]; edgeId++) {
					auto& edge = compressedGraph.edges[edgeId];
					if (turnId + edge.turns > maxTurn) continue;
					size_t prevIndex = distanceArrayOffset[turnId] + edge.from;
					size_t toIndex = distanceArrayOffset[turnId + edge.turns] + edge.to;
					int64 nextProfit = result[prevIndex].offsetProfit + edge.cost;
					if (nextProfit <= result[toIndex].offsetProfit) continue;
					result[toIndex].offsetProfit = nextProfit;

					// 最初のターンは具体的に行動を入力、それ以外は引き継ぐ
					if (turnId == 0) {
						switch (edge.type) {
						case 0:
							result[toIndex].firstMove = ShortenMove::Move(edge.direction);
							break;
						case 1:
							result[toIndex].firstMove = ShortenMove::Construct(edge.direction);
							break;
						case 2:
						case 3:
							result[toIndex].firstMove = ShortenMove::Destroy(edge.direction);
							break;
						}
					}
					else {
						result[toIndex].firstMove = result[prevIndex].firstMove;
					}
				}
			}
		}

		return result;
	}


}

