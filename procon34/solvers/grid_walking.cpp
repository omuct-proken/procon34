#pragma once
#include "grid_walking.hpp"

namespace Procon34 {


	GridWalking::GridWalking(
		std::shared_ptr<const GameState> _game,
		Array<Grid<int64>> _visitingProfit
	)
		: game(_game)
		, visitingProfit(std::move(_visitingProfit))
		, boardSize(_game->getBoard()->getSize())
	{
		capableTurnCount = (int32)visitingProfit.size() - 1;
		for(size_t i=0; i< visitingProfit.size(); i++){
			if (visitingProfit[i].size() != boardSize) {
				throw Error(U"Error at GridWalking::Answer GridWalking::solve(Agent agent, int32 maxTurnCount) : visitingProfit[i].size() != boardSize , i = {}"_fmt(i));
			}
		}
	}

	GridWalking::Answer GridWalking::solve(Agent agent, int32 maxTurnCount) {
		if (maxTurnCount > capableTurnCount) throw Error(U"Error at GridWalking::Answer GridWalking::solve(Agent agent, int32 maxTurnCount) : maxTurnCount > capableTurnCount");

		auto board = game->getBoard();
		ShortPathAnswer defaultAnswer = ShortPathAnswer{ .firstMove = AgentMove::GetStay(agent), .profit = INT64_MIN / 3 };
		ShortPathAnswer answerAtInitialPosition = ShortPathAnswer{ .firstMove = AgentMove::GetStay(agent), .profit = 0 };
		Grid<int32> massRegistoration(boardSize, -1);
		Array<Array<ShortPathAnswer>> buffer(maxTurnCount + 1);

		Array<BoardPos> positions;

		Array<Array<std::pair<BoardPos, ShortPathAnswer>>> nextBuffer(2);
		nextBuffer[0].push_back(std::make_pair(agent.pos, answerAtInitialPosition));

		for (int32 turn = 0; turn <= maxTurnCount; turn++) {
			auto nowBuffer = std::move(nextBuffer[0]);
			for (size_t i = 1; i < nextBuffer.size(); i++) std::swap(nextBuffer[i - 1], nextBuffer[i]);

			// 今のターンのコスト最善の方法を記録
			// 新しく登場した座標を登録
			if(turn != 0) buffer[turn].assign(buffer[turn - 1].size(), defaultAnswer);
			for (auto& [pos, ans] : nowBuffer) {
				int massid = massRegistoration[pos.asPoint()];
				if (massid < 0) {
					massRegistoration[pos.asPoint()] = (int32)positions.size();
					positions.push_back(pos);
					buffer[turn].push_back(ans);
				}
				else {
					if (ans.profit <= buffer[turn][massid].profit) continue;
					buffer[turn][massid] = ans;
				}
			}

			// 次の手も考慮しなければならないなら、
			//     記録した解をもとにつぎの手を求める
			if (turn < maxTurnCount) {

				for (size_t nowPostitionIndex = 0; nowPostitionIndex < positions.size(); nowPostitionIndex++) {
					auto nowPosition = positions[nowPostitionIndex];
					ShortPathAnswer prevAns = buffer[turn][nowPostitionIndex];

					for (int32 directionVal = 0; directionVal < 8; directionVal++) {

						auto direction = MoveDirection(directionVal);
						auto newPosition = nowPosition.movedAlong(direction);
						if (!board->isOnBoard(newPosition)) continue;   // 盤外に出たらだめ
						auto newPositionMass = board->getMass(newPosition);
						if (newPositionMass.biome == MassBiome::Pond) continue; // 池だったらだめ

						bool hasOpponentWall = newPositionMass.hasWallOf(game->OpponentOf(agent.marker.color));
						if (!direction.is4Direction() && hasOpponentWall) continue; // 4 方向以外で、敵の壁には進めない
						ShortPathAnswer nextAnswer = ShortPathAnswer{ .firstMove = buffer[turn][nowPostitionIndex].firstMove, .profit = 0 };

						// 消費ターン数
						int32 consumeTurns = hasOpponentWall ? 2 : 1;
						if (turn + consumeTurns > maxTurnCount) continue; // ターン数オーバー

						// firstMove を計算
						if (turn == 0) {
							if (hasOpponentWall) {
								nextAnswer.firstMove = AgentMove::GetDestroy(agent, direction);
							}
							else {
								nextAnswer.firstMove = AgentMove::GetMove(agent, direction);
							}
						}
						else {
							nextAnswer.firstMove = buffer[turn][nowPostitionIndex].firstMove;
						}

						// 利得計算
						nextAnswer.profit = prevAns.profit + visitingProfit[turn + consumeTurns][newPosition.asPoint()];

						// 登録
						nextBuffer[consumeTurns - 1].push_back(std::make_pair(newPosition, nextAnswer));

					}
				}

			}

		}

		GridWalking::Answer resultBuffer;
		resultBuffer.maxTurnCount = maxTurnCount;
		resultBuffer.shortPath = std::move(buffer);
		resultBuffer.posistions = std::move(positions);
		resultBuffer.positionToListIndex = std::move(massRegistoration);
		return resultBuffer;
	}

}

