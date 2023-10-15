#include "../stdafx.h"
#include "solver_list.hpp"
#include "solver_main2.hpp"
#include "construct_wall_path_2.hpp"
#include "shorten_move.hpp"
#include "thread_pool.hpp"

namespace Procon34 {


	namespace Solvers {

		// 盤面の状態から指示を作る
		TurnInstruction MainSolution2::operator()(BoxPtr<const GameState> state) {
			int32 myTurnsLeft = (state->getInitialState()->turnCount - state->getTurnIndex() + 1) / 2;
			auto myColor = state->whosTurn();
			auto opponentColor = state->OpponentOf(myColor);
			auto myAgents = state->getAgents(myColor);
			auto opponentAgents = state->getAgents(opponentColor);

			auto board = state->getBoard();
			auto boardSize = board->getSize();
			int32 turnCount = Min(10, myTurnsLeft);

			// ----------------------------------------------
			//   パラメータ

			auto visitingProfit = Array<Grid<int64>>(turnCount + 1, Grid<int64>(boardSize, 0));
			auto territoryProfit = Grid<int64>(boardSize, 0);
			auto wallProfit = Grid<int64>(boardSize, 0);
			auto enabledDifference = WallPath2::EnabledDifferenceDefaultValue();
			auto turnProfit = Array<int64>(turnCount + 1);
			auto wallProfitPositive = Grid<int64>(boardSize, 0);

			auto distanceFromOpponentAgnet = [&](BoardPos pos) -> int32 {
				int32 res = 1001001;
				for (auto agent : opponentAgents) {
					res = std::min(res, std::abs(agent.pos.r - pos.r) + std::abs(agent.pos.c - pos.c));
				}
				return res;
				};

			for (int32 i = 0; i <= turnCount; i++) turnProfit[i] = -i;

			auto adjacentToMyAgent = Grid<int32>(boardSize, 0);
			auto adjacent4ToMyWall = Grid<int32>(boardSize, 0);
			for (int32 r = 0; r < board->getHeight(); r++) for (int32 c = 0; c < board->getWidth(); c++) {
				auto boardPos = BoardPos(r, c);
				auto thisMassAgent = board->getMass(boardPos).agent;
				adjacentToMyAgent[boardPos.asPoint()]++;
				if (thisMassAgent.has_value() && thisMassAgent->color == myColor) {
					for (int32 d = 0; d < 8; d++) {
						auto newPos = boardPos.movedAlong(MoveDirection(d));
						if (!board->isOnBoard(newPos)) continue;
						adjacentToMyAgent[newPos.asPoint()]++;
					}
				}
				for (int32 d = 0; d < 8; d++) {
					if (!MoveDirection(d).is4Direction()) continue;
					auto newPos = boardPos.movedAlong(MoveDirection(d));
					if (!board->isOnBoard(newPos)) continue;
					if (board->getMass(newPos).hasWallOf(myColor)) adjacent4ToMyWall[boardPos.asPoint()]++;
				}
			}

			for (int32 r = 0; r < board->getHeight(); r++) for (int32 c = 0; c < board->getWidth(); c++) {
				auto boardPos = BoardPos(r, c);
				int64 tProfit = 1000;
				int64 wProfit = -500;
				int64 wpProfit = 500;
				Array<int64> vProfit(turnCount + 1);

				int32 adjacentBlockades = 0;
				if (r == 0 || board->getMass(BoardPos(r - 1, c)).biome == MassBiome::Pond) adjacentBlockades++;
				if (r == board->getHeight() - 1 || board->getMass(BoardPos(r + 1, c)).biome == MassBiome::Pond) adjacentBlockades++;
				if (c == 0 || board->getMass(BoardPos(r, c - 1)).biome == MassBiome::Pond) adjacentBlockades++;
				if (c == board->getWidth() - 1 || board->getMass(BoardPos(r, c + 1)).biome == MassBiome::Pond) adjacentBlockades++;

				auto mass = board->getMass(boardPos);
				if (mass.hasWallOf(opponentColor)) {
					tProfit = 500;
					wProfit = 300;
					wpProfit = 1000;
				}
				if (adjacentBlockades >= 1) {
					wpProfit += 500 * adjacentBlockades;
				}
				if (mass.hasWallOf(myColor)) {
					wProfit = std::min(wProfit, (int64)-100);
					wpProfit = -1000;
				}
				if (state->isAreaOf(myColor, boardPos)) {
					wProfit = std::min(wProfit, (int64)-10000);
					wpProfit = -10000;
				}
				wpProfit += adjacent4ToMyWall[boardPos.asPoint()] * adjacent4ToMyWall[boardPos.asPoint()] * (-800);

				tProfit *= 5;
				wProfit *= 5;



				if (mass.hasWallOf(myColor)) tProfit = 0; // 無条件


				// サンプル
				wpProfit = 0;
				wProfit = -10;
				tProfit = 100;
				// サンプル　終わり

				if (adjacentToMyAgent[boardPos.asPoint()] > 1) {
					for (size_t t = 0; t < 3; t++) if (t < vProfit.size()) {
						vProfit[t] = -1001001001001;
					}
				}
				int32 dist = distanceFromOpponentAgnet(boardPos);
				if (dist == 0) {
					for (int32 t = 0; t <= turnCount; t++) vProfit[t] = -1001001001001;
					wProfit = -1001001001001;
					wpProfit = -1001001001001;
				}

				territoryProfit[boardPos.asPoint()] = tProfit;
				wallProfit[boardPos.asPoint()] = wProfit;
				wallProfitPositive[boardPos.asPoint()] = wpProfit;
				for (int32 t = 0; t <= turnCount; t++) visitingProfit[t][boardPos.asPoint()] = vProfit[t];
			}

			// 絶対に守らないといけない制約が、そこにある
			for (int32 r = 0; r < board->getHeight(); r++) for (int32 c = 0; c < board->getWidth(); c++) {
				auto boardPos = BoardPos(r, c);
				auto mass = board->getMass(boardPos);

				// 自分の壁があるマスの陣地利得は 0
				if (mass.hasWallOf(myColor)) {
					territoryProfit[boardPos.asPoint()] = 0;
				}
				// 自分の陣地であるマスの陣地利得は 0
				if (state->isAreaOf(myColor, boardPos)) {
					territoryProfit[boardPos.asPoint()] = 0;
				}
			};

			// ----------------------------------------------
			//   サブルーチン

			auto gridWalking = std::make_shared<GridWalking>(state, visitingProfit);
			auto diagGraph = std::make_shared<DiagonalGraph>(state, territoryProfit);
			auto wallPathA = std::make_shared<WallPath2>(diagGraph, state, wallProfit, enabledDifference, false);
			auto wallPathB = std::make_shared<WallPath2>(diagGraph, state, wallProfit, enabledDifference, true);
			auto constructWallPath = ConstructWallPath2(diagGraph, gridWalking, { wallPathA, wallPathB });

			auto diagGraphZero = std::make_shared<DiagonalGraph>(state, Grid<int64>(boardSize, 0));
			auto wallPathC = std::make_shared<WallPath2>(diagGraphZero, state, wallProfitPositive, enabledDifference, false);
			auto constructWallPathPositive = ConstructWallPath2(diagGraphZero, gridWalking, { wallPathC });

			auto wallPaths = myAgents.map([&](Agent agent) { return constructWallPath.solve(agent, turnCount, turnProfit); });
			auto wallPathsPositive = myAgents.map([&](Agent agent) { return constructWallPathPositive.solve(agent, turnCount, turnProfit); });


			// ----------------------------------------------

			static const int64 NegativeInf = -1001001001001;
			struct SearchNode {
				int64 profit = NegativeInf;
				ShortenMoveSet firstMoves;
			};
			auto maxProfitCycle = Array<SearchNode>((size_t)1 << myAgents.size());

			auto threadPool = ThreadPool::Construct(10);

			for (int32 maxAgentIndex = 0; maxAgentIndex < (int32)myAgents.size(); maxAgentIndex++) {
				int32 searchSize = constructWallPath.getNumberOfBases();

				auto canStart = Array<int32>(searchSize, 0);
				for (auto& a : wallPaths[maxAgentIndex]) canStart[a.from] = 1;

				Array<Array<SearchNode>> maxProfitCycleBuffer(canStart.size(), Array<SearchNode>((size_t)1 << myAgents.size()));

				for (int32 s = 0; s < (int32)canStart.size(); s++) if(canStart[s] == 1) {
					auto task = [&, s] (uint32){

						auto dp = Array<Array<SearchNode>>((size_t)1 << maxAgentIndex, Array<SearchNode>(searchSize));
						for (auto& a : wallPaths[maxAgentIndex]) if (a.from == s) if (dp[0][a.to].profit < a.profit) {
							dp[0][a.to].profit = a.profit;
							dp[0][a.to].firstMoves.setAt(maxAgentIndex, ShortenMove::Encode(a.firstMove));
						}
						for (int32 j = 0; j < (int32)dp.size(); j++) {
							//maxProfitCycle[j + (1 << maxAgentIndex)] = *std::max_element(dp[j].begin(), dp[j].end(),
							//	[](const SearchNode& l, const SearchNode& r) { return l.profit < r.profit; });
							{
								auto maxProfitTmp = dp[j][s];
								if (maxProfitCycleBuffer[s][j + (1 << maxAgentIndex)].profit < maxProfitTmp.profit) {
									maxProfitCycleBuffer[s][j + (1 << maxAgentIndex)] = maxProfitTmp;
								}
							}
							for (int32 ag = 0; ag < maxAgentIndex; ag++) if (!(j & (1 << ag))) {
								for (auto& a : wallPaths[ag]) {
									auto& tov = dp[j | (1 << ag)][a.to];
									int64 nxprofit = dp[j][a.from].profit + a.profit;
									if (tov.profit < nxprofit) {
										tov.profit = nxprofit;
										tov.firstMoves = dp[j][a.from].firstMoves;
										tov.firstMoves.setAt(ag, ShortenMove::Encode(a.firstMove));
									}
								}
							}
						}

					};

					threadPool->pushTask(task);

				}

				threadPool->sync();

				for (auto& a : maxProfitCycleBuffer) {
					for (size_t i = 0; i < maxProfitCycle.size(); i++) {
						if (maxProfitCycle[i].profit < a[i].profit) maxProfitCycle[i] = a[i];
					}
				}

			}

			auto maxProfitPlan = maxProfitCycle;

			for (int32 agentId = 0; agentId < (int32)myAgents.size(); agentId++) {
				for (auto a : wallPathsPositive[agentId]) {
					SearchNode buf;
					buf.profit = a.profit;
					buf.firstMoves.setAt(agentId, ShortenMove::Encode(a.firstMove));
					if (maxProfitPlan[(size_t)1 << agentId].profit < buf.profit) {
						maxProfitPlan[(size_t)1 << agentId] = buf;
					}
				}
			}

			for (size_t i = 0; i < (size_t)1 << myAgents.size(); i++) {
				for (size_t j = (i - 1) & i; j != 0; j = (j - 1) & i) {
					if (maxProfitPlan[i].profit < maxProfitPlan[j].profit + maxProfitPlan[i - j].profit) {
						maxProfitPlan[i].profit = maxProfitPlan[j].profit + maxProfitPlan[i - j].profit;
						maxProfitPlan[i].firstMoves = maxProfitPlan[i - j].firstMoves;
						for (int32 b = 0; b < myAgents.size(); b++) if ((j >> b) & 1) {
							maxProfitPlan[i].firstMoves.setAt(b, maxProfitPlan[j].firstMoves.getAt(b));
						}
					}
				}
			}

			// 次の一手を登録
			auto result = TurnInstruction(state, myColor);
			auto planToExec = maxProfitPlan.back();

			for (int32 i = 0; i < myAgents.size(); i++) {
				auto agentMove = ShortenMove::Decode(planToExec.firstMoves.getAt(i), myAgents[i]);
				result.insert(agentMove);
			}

			Console << U"Turn #{} :"_fmt(state->getTurnIndex());
			//maxProfitPlan.each_index([&](size_t i, SearchNode node) {
			//	Console << U"   cand . i = {} , profit = {}"_fmt(i, node.profit);
			//});
			for (int32 i = 0; i < myAgents.size(); i++) {
				auto agentMove = ShortenMove::Decode(planToExec.firstMoves.getAt(i), myAgents[i]);
				if (agentMove.isStay()) Console << U"({}, {}) -> Stay"_fmt(myAgents[i].pos.r, myAgents[i].pos.c);
				if (agentMove.isMove()) Console << U"({}, {}) -> Move {}"_fmt(myAgents[i].pos.r, myAgents[i].pos.c, agentMove.asMove().dir.value());
				if (agentMove.isConstruct()) Console << U"({}, {}) -> Construct {}"_fmt(myAgents[i].pos.r, myAgents[i].pos.c, agentMove.asConstruct().dir.value());
				if (agentMove.isDestroy()) Console << U"({}, {}) -> Destroy {}"_fmt(myAgents[i].pos.r, myAgents[i].pos.c, agentMove.asDestroy().dir.value());
			}
			Console << U" ---- ---- ---- ----";

			return result;


		}

		String MainSolution2::name() { return U"戦略 アップデート"; }

	}


} // namespace Procon34
