#pragma once
#include "diag_graph.hpp"
#include "../dsu_fast.hpp"

namespace Procon34 {


	DiagonalGraph::DiagonalGraph(
		std::shared_ptr<const GameState> game,
		Grid<int64> territoryScore
	)
		: m_game(game)
		, m_territoryScore(std::move(territoryScore))
	{
		auto board = m_game->getBoard();
		auto boardSize = board->getSize();
		int32 height = board->getHeight();
		int32 width = board->getWidth();

		auto myColor = game->whosTurn();
		// auto opponentColor = game->OpponentOf(myColor);

		m_diffDown.assign(Size(width + 1, height), 0);
		for (int32 y = 0; y < height; y++) for (int32 x = 0; x < width; x++) {
			m_diffDown[Point(x + 1, y)] = m_diffDown[Point(x, y)] + m_territoryScore[Point(x, y)];
		}

		// m_toBaseId
		// m_profitToGoToBasePos
		// m_idToBasePos
		// を計算する
		{
			nachia::DsuFast dsu((height + 1) * (width + 1));
			auto dsuIndex = [w = width + 1](int32 y, int32 x) -> int32 { return w * y + x; };
			for (int32 y = 0; y < height; y++) for (int32 x = 0; x < width; x++) {
				if (board->getMass(BoardPos(y, x)).hasWallOf(myColor)) {
					dsu.merge(dsuIndex(y, x), dsuIndex(y, x + 1));
					dsu.merge(dsuIndex(y, x), dsuIndex(y + 1, x));
					dsu.merge(dsuIndex(y, x), dsuIndex(y + 1, x + 1));
				}
			}
			m_toBaseId.assign(Point(boardSize.x + 1, boardSize.y + 1), -1);
			m_profitToGoToBasePos.assign(Point(boardSize.x + 1, boardSize.y + 1), 0);
			m_idToBasePos = {};
			for (int32 y = 0; y < height + 1; y++) for (int32 x = 0; x < width + 1; x++) {
				if (dsu.leader(dsuIndex(y, x)) == dsuIndex(y, x)) {
					auto startPos = Point(x, y);
					int32 newId = (int32)m_idToBasePos.size();
					m_idToBasePos.push_back(startPos);
					m_toBaseId[startPos] = newId;
					m_profitToGoToBasePos[startPos] = 0;
					Array<Point> que;
					que.push_back(startPos);
					for (size_t i = 0; i < que.size(); i++) {
						auto v = que[i];
						if (0 < v.x) { // 左。
							auto w = Point(v.x - 1, v.y);
							if (dsu.leader(dsuIndex(y, x)) == dsu.leader(dsuIndex(w.y, w.x)) && m_toBaseId[w] == -1) {
								que.push_back(w);
								m_toBaseId[w] = m_toBaseId[v];
								m_profitToGoToBasePos[w] = m_profitToGoToBasePos[v];
							}
						}
						if (v.x < width) { // 右。領域の角の座標なので、 x は x = width まで動く
							auto w = Point(v.x + 1, v.y);
							if (dsu.leader(dsuIndex(y, x)) == dsu.leader(dsuIndex(w.y, w.x)) && m_toBaseId[w] == -1) {
								que.push_back(w);
								m_toBaseId[w] = m_toBaseId[v];
								m_profitToGoToBasePos[w] = m_profitToGoToBasePos[v];
							}
						}
						if (0 < v.y) { // 上。
							auto w = Point(v.x, v.y - 1);
							if (dsu.leader(dsuIndex(y, x)) == dsu.leader(dsuIndex(w.y, w.x)) && m_toBaseId[w] == -1) {
								que.push_back(w);
								m_toBaseId[w] = m_toBaseId[v];
								m_profitToGoToBasePos[w] = m_profitToGoToBasePos[v] + m_diffDown[Point(v.x, v.y - 1)]; // 上下に動くので、点数が変わる
							}
						}
						if (v.y < height) { //下。 領域の角の座標なので、 y は y = height まで動く
							auto w = Point(v.x, v.y + 1);
							if (dsu.leader(dsuIndex(y, x)) == dsu.leader(dsuIndex(w.y, w.x)) && m_toBaseId[w] == -1) {
								que.push_back(w);
								m_toBaseId[w] = m_toBaseId[v];
								m_profitToGoToBasePos[w] = m_profitToGoToBasePos[v] - m_diffDown[v]; // 上下に動くので、点数が変わる
							}
						}
					}
				}
			}
		}

		// m_wallCandidateId
		// m_wallCandidatePos
		// m_wallAccess
		// を計算する
		{
			m_wallCandidateId.assign(Size(width, height), -1);
			int32 numId = 0;
			for (int32 y = 0; y < height; y++) for (int32 x = 0; x < width; x++) {
				auto pos = BoardPos(y, x);
				if (!board->getMass(pos).hasWallOf(myColor)
					&& board->getMass(pos).biome != MassBiome::Castle
				) {
					m_wallCandidateId[pos.asPoint()] = numId++;
					m_wallCandidatePos.push_back(pos);
				}
			}

			// 半時計回りで囲った領域が正に寄与するから、
			// これから建設する壁の周囲は時計回りに回らなければならない。
			m_wallAccess.assign(numId, Array<WallAccess>{});
			for (int32 i = 0; i < numId; i++) {
				auto pos = m_wallCandidatePos[i].asPoint();
				std::map<int32, int64> buffer;
				//
				//    0---1
				//    |   |
				//    3---2
				//
				int32 id0 = m_toBaseId[pos + Point(0, 0)]; int64 diff0 = m_profitToGoToBasePos[pos + Point(0, 0)];
				int32 id1 = m_toBaseId[pos + Point(1, 0)]; int64 diff1 = m_profitToGoToBasePos[pos + Point(1, 0)];
				int32 id2 = m_toBaseId[pos + Point(1, 1)]; int64 diff2 = m_profitToGoToBasePos[pos + Point(1, 1)];
				int32 id3 = m_toBaseId[pos + Point(0, 1)]; int64 diff3 = m_profitToGoToBasePos[pos + Point(0, 1)];
				int64 d01 = 0;
				int64 d12 = m_diffDown[pos + Point(1, 0)]; // (1,0) -> (1,1)
				int64 d23 = 0;
				int64 d30 = -m_diffDown[pos + Point(0, 0)]; // (0,1) -> (0,0)
				//if (id0 != id1) {
					m_wallAccess[i].push_back({ id0, id1, -diff0 + diff1 + d01 });
					m_wallAccess[i].push_back({ id1, id0, -diff1 + diff0 + d12 + d23 + d30 });
				//}
				//if (id0 != id2) {
					m_wallAccess[i].push_back({ id0, id2, -diff0 + diff2 + d01 + d12 });
					m_wallAccess[i].push_back({ id2, id0, -diff2 + diff0 + d23 + d30 });
				//}
				//if (id0 != id3) {
					m_wallAccess[i].push_back({ id0, id3, -diff0 + diff3 + d01 + d12 + d23 });
					m_wallAccess[i].push_back({ id3, id0, -diff3 + diff0 + d30 });
				//}
				//if (id1 != id2) {
					m_wallAccess[i].push_back({ id1, id2, -diff1 + diff2 + d12 });
					m_wallAccess[i].push_back({ id2, id1, -diff2 + diff1 + d23 + d30 + d01 });
				//}
				//if (id1 != id3) {
					m_wallAccess[i].push_back({ id1, id3, -diff1 + diff3 + d12 + d23 });
					m_wallAccess[i].push_back({ id3, id1, -diff3 + diff1 + d30 + d01 });
				//}
				//if (id2 != id3) {
					m_wallAccess[i].push_back({ id2, id3, -diff2 + diff3 + d23 });
					m_wallAccess[i].push_back({ id3, id2, -diff3 + diff2 + d30 + d01 + d12 });
				//}
			}
		}

	}



	int32 DiagonalGraph::numberOfBases() const {
		return (int32)m_idToBasePos.size();
	}



}

