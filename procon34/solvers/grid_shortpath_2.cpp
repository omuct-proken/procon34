#pragma once
#include "grid_shortpath_2.hpp"
#include "../game_state.hpp"

namespace Procon34 {

	WallPath2::NodeId WallPath2::getNodeId(BoardPos agentPos, int32 baseId) {
		auto& ref = nodesIndexedByAgentPos[agentPos.asPoint()];
		auto iter = std::lower_bound(ref.begin(), ref.end(), std::make_pair(baseId, -1));
		if (iter == ref.end()) return -1;
		if (iter->first != baseId) return -1;
		return iter->second;
	}

	bool WallPath2::isReversed() const {
		return m_asReversed;
	}

	WallPath2::WallPath2(
		std::shared_ptr<DiagonalGraph> _diagGraph,
		std::shared_ptr<const GameState> _game,
		const Grid<int64>& _wallScore,
		Array<Point> _enabledDifference,
		bool asReversed
	)
		: diagGraph(_diagGraph)
		, game(_game)
		, wallScore(_wallScore)
		, enabledDifference(_enabledDifference)
		, m_asReversed(asReversed)
	{

		// 既存の壁の連結成分の計算

		auto board = game->getBoard();
		auto myColor = game->whosTurn();
		auto opponentColor = game->OpponentOf(myColor);
		int32 height = board->getHeight();
		int32 width = board->getWidth();
		Size gridSize = board->getSize();
		assert(wallScore.size() == gridSize);


		// ノードの集合の構築

		// [職人の座標][baseId] -> nodeId
		Grid<std::map<int32, NodeId>> nodeMapping(gridSize);
		int32 nextInsertedNodeId = 0;

		auto nodeInfoToId = [&](BoardPos agentPos, int32 baseId) -> int32 {
			if (!board->isOnBoard(agentPos)) return -1;
			if (!(0 <= baseId)) return -1;
			auto agentPosP = agentPos.asPoint();
			auto iter = nodeMapping[agentPosP].find(baseId);
			if (iter == nodeMapping[agentPosP].end()) return -1;
			return iter->second;
			};
		auto insertNodeId = [&](BoardPos agentPos, int32 baseId) -> int32 {
			if (!board->isOnBoard(agentPos)) return -1;
			if (!(0 <= baseId)) return -1;
			auto agentPosP = agentPos.asPoint();
			auto iter = nodeMapping[agentPosP].find(baseId);
			if (iter != nodeMapping[agentPosP].end()) return iter->second;
			nodeMapping[agentPosP][baseId] = nextInsertedNodeId;
			nextInsertedNodeId++;
			return nextInsertedNodeId - 1;
			};

		{

			for (int32 agentY = 0; agentY < height; agentY++) for (int32 agentX = 0; agentX < width; agentX++) {
				auto agentPos = BoardPos(agentY, agentX);
				if (board->getMass(agentPos).biome == MassBiome::Pond) continue; // 侵入可能？

				for (auto difference : enabledDifference) {
					Point basePos = Point(agentX + difference.x, agentY + difference.y);
					if (basePos.x < 0 || basePos.y < 0 || basePos.x > width || basePos.y > height) continue;
					insertNodeId(agentPos, diagGraph->m_toBaseId[basePos]);
				}
			}

			nodes.resize(nextInsertedNodeId);
			for (int32 agentY = 0; agentY < height; agentY++) for (int32 agentX = 0; agentX < width; agentX++) {
				auto agentPos = BoardPos(agentY, agentX);
				for (auto node : nodeMapping[agentPos.asPoint()]) {
					nodes[node.second].agentPos = agentPos;
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

		nodesIndexedByBaseid.resize(diagGraph->numberOfBases());
		for (auto& v : nodes) {
			nodesIndexedByBaseid[v.baseId].push_back(v.nodeId);
		}




		// 職人が移動する遷移の計算
		for (auto node : nodes) {
			auto agentPos = node.agentPos;

			for (int32 moveDirectionVal = 0; moveDirectionVal < 8; moveDirectionVal++) {
				auto moveDirection = MoveDirection(moveDirectionVal);
				auto newAgentPos = agentPos.movedAlong(moveDirection);
				int32 nextNodeId = nodeInfoToId(newAgentPos, node.baseId);
				if (nextNodeId < 0) continue;

				EdgeDesc tmp;
				tmp.cost = 0;

				if (board->getMass(newAgentPos).hasWallOf(GameState::OpponentOf(myColor)) // 敵の壁がある
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

		}


		// 壁を設置する遷移の計算
		for (int32 agentY = 0; agentY < height; agentY++) for (int32 agentX = 0; agentX < width; agentX++) {
			auto agentPos = BoardPos(agentY, agentX);
			for (int32 wallDirectionVal = 0; wallDirectionVal < 8; wallDirectionVal += 2) {
				assert(MoveDirection(wallDirectionVal).is4Direction());

				auto wallPos = agentPos.movedAlong(MoveDirection(wallDirectionVal)); // これから建設するマス
				if (!board->isOnBoard(wallPos)) continue; // 盤面内？
				if (board->getMass(wallPos).biome == MassBiome::Castle) continue; // 建設可能？
				if (board->getMass(wallPos).hasWallOf(myColor)) continue; // 未建設？
				if (diagGraph->m_wallCandidateId[wallPos.asPoint()] < 0) continue; // diagGraph で、建設可能フラグが立っている？

				int64 thisWallProfit = wallScore[wallPos.asPoint()];
				bool existOpponentWall = board->getMass(wallPos).hasWallOf(opponentColor);

				for (auto [fromBase, toBase, profitDiff] : diagGraph->m_wallAccess[diagGraph->m_wallCandidateId[wallPos.asPoint()]]) {
					int32 fromNode = nodeInfoToId(agentPos, fromBase);
					int32 toNode = nodeInfoToId(agentPos, toBase);
					if (fromNode < 0 || toNode < 0 || fromNode == toNode) continue;
					int64 profit = thisWallProfit + (profitDiff * (asReversed ? -1 : 1));

					edges.push_back(EdgeDesc{
						.from = fromNode,
						.to = toNode,
						.type = existOpponentWall ? 3 : 1,
						.direction = MoveDirection(wallDirectionVal),
						.turns = existOpponentWall ? 2 : 1,
						.cost = profit
					});
				}
			}
		}

		edges.sort_by([](const EdgeDesc& l, const EdgeDesc& r) { return l.from < r.from; });
		edgesSeparators.assign(nodes.size() + 1, 0);
		for (auto& e : edges) edgesSeparators[e.from + 1]++;
		for (size_t i = 0; i < nodes.size(); i++) edgesSeparators[i + 1] += edgesSeparators[i];

	}

	Array<Point> WallPath2::EnabledDifferenceDefaultValue() {
		Array<Point> res;
		for (int32 dx = -1; dx <= 2; dx++) for (int32 dy = -1; dy <= 2; dy++) if (abs(dx * 2 - 1) + abs(dy * 2 - 1) <= 5) {
			res.push_back(Point(dx, dy));
		}
		return res;
	}


	WallPath2::CompressedByDistance WallPath2::compressGraphsByDistance(int32 maxDistance, Array<std::pair<int32, int32>> starters) const {
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


	Array<WallPath2::MovingState> WallPath2::solve(int32 maxTurn, Array<MovingState> starters) {
		auto callCompressGraphsByDistance = [&]() -> WallPath2::CompressedByDistance {
			Array<std::pair<int32, int32>> newStarters;
			for (auto& start : starters) {
				newStarters.push_back(std::make_pair(start.nodeId, start.turnCount));
			}
			return compressGraphsByDistance(maxTurn, std::move(newStarters));
		};
		auto compressedGraph = callCompressGraphsByDistance();

		Array<size_t> distanceArrayOffset(maxTurn + 2, 0);
		for (int32 d = 0; d <= maxTurn; d++) distanceArrayOffset[d + 1] = distanceArrayOffset[d] + compressedGraph.distanceSeparator[d + 1];

		WallPath2::MovingState ini;
		ini.offsetProfit = -1001001001001001;
		Array<WallPath2::MovingState> result(distanceArrayOffset.back(), ini);

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

