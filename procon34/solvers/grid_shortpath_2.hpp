#pragma once
#include "../stdafx.h"
#include "../game_state.hpp"
#include "shorten_move.hpp"
#include "diag_graph.hpp"

namespace Procon34 {

	// 定義

	// base : マスの角に対して振られる。自分の壁で連結になっているところは同一視する。
	// agentPos : 職人がいる座標
	// node : base と agentPos の組に対して振られる。

	struct WallPath2 {

		using NodeId = int32;

		struct NodeDesc {
			BoardPos agentPos;
			int32 baseId;
			NodeId nodeId = 0;
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
		std::shared_ptr<DiagonalGraph> diagGraph;
		std::shared_ptr<const GameState> game;
		Grid<int64> wallScore;
		Array<Point> enabledDifference;

		Array<NodeDesc> nodes;
		Array<EdgeDesc> edges;
		Grid<Array<std::pair<int32, NodeId>>> nodesIndexedByAgentPos;
		Array<Array<NodeId>> nodesIndexedByBaseid;
		Array<int32> edgesSeparators;
		bool m_asReversed;

		NodeId getNodeId(BoardPos agentPos, int32 baseId);

		bool isReversed() const;

		WallPath2(
			std::shared_ptr<DiagonalGraph> _diagGraph,
			std::shared_ptr<const GameState> _game,
			const Grid<int64>& _wallScore,
			Array<Point> _enabledDifference,
			bool asReversed = true
		);

		static Array<Point> EnabledDifferenceDefaultValue();


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

