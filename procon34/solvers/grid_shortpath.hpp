#pragma once
#include "../stdafx.h"
#include "../game_state.hpp"
#include "shorten_move.hpp"

namespace Procon34 {

	struct WallPath {

		using NodeId = int32;

		struct NodeDesc {
			BoardPos agentPos;
			BoardPos wallPos;
			NodeId nodeId = 0;
			int32 baseId;
		};

		struct EdgeDesc {
			NodeId from;
			NodeId to;
			int32 type;
			MoveDirection direction;
			int32 turns;
			int64 cost;
		};

		// type 0 : 職人が移動
		// type 1 : 壁を建設
		// type 2 : 壁を破壊して職人が移動
		// type 3 : 壁を破壊して壁を建設


		// input
		std::shared_ptr<const GameState> game;
		Grid<int64> territoryScore;
		Grid<int64> wallScore;
		Array<Point> enabledDifference;

		Grid<int64> territoryScoreCum;

		// 壁の連結成分あるいは壁を置けるマスには代表のマス base が設定される。
		Grid<int32> baseId;
		Array<BoardPos> basePos;
		// base からそのマスに、 CCW 向きに進んだ時の利得の増加量
		Grid<int64> differenceFromBase;

		Array<NodeDesc> nodes;
		Array<EdgeDesc> edges;
		Grid<Array<std::pair<int32, NodeId>>> nodesIndexedByAgentPos;
		Array<Array<NodeId>> nodesIndexedByWallPos;
		Array<int32> edgesSeparators;
		bool m_asReversed;

		// 隣接するマスに順に壁を置いた時の、見込み領域の利得
		int64 territoryScoreDiffCcw(BoardPos from, BoardPos to);
		int64 territoryScoreDiffCw(BoardPos from, BoardPos to);

		NodeId getNodeId(BoardPos agentPos, BoardPos wallPos);

		bool isReversed() const;

		WallPath(
			std::shared_ptr<const GameState> _game,
			const Grid<int64>& _territoryScore,
			const Grid<int64>& _wallScore,
			Array<Point> _enabledDifference,
			bool asReversed = true
		);

		static Array<Point> EnabledDifferenceDefaultValue();


		int32 getBaseOf(BoardPos pos) const;
		int32 getNumberOfBaseWallPositions() const;


		Array<AgentMove> doOneCycle(Agent agent, int32 turnCount = 20);


		struct CompressedByDistance {
			Array<int32> nodesMapping; // 距離が小さい順にならべたもの。圧縮したグラフではこの順にノードの番号を 0,1,... と振りなおす。
			Array<int32> distanceSeparator; // [d+1] = 距離 d 以下で行けるノードの個数。ノードのリストは nodesMapping から得られる
			Array<EdgeDesc> edges; // 圧縮してノード番号を振りなおしたあとの辺
			Array<int32> edgesSeparator; // [d] = 距離 d 以下のノードから出る辺の個数
		};

		// starters : list of (node id, offset distance)
		CompressedByDistance compressGraphsByDistance(int32 maxDistance, Array<std::pair<int32, int32>> starters) const;

		struct MovingState {
			int32 nodeId;
			int32 turnCount;
			ShortenMove::Type firstMove;
			int64 offsetProfit;
		};

		Array<MovingState> solve(int32 maxTurn, Array<MovingState> starters);

	};

}

