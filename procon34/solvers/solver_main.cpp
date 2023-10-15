#include "../stdafx.h"
#include "solver_list.hpp"
#include "solver_main.hpp"
#include "construct_wall_path.hpp"
#include "shorten_move.hpp"

namespace Procon34 {


	namespace Solvers {

		// 盤面の状態から指示を作る
		TurnInstruction MainSolution::operator()(BoxPtr<const GameState> state) {
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
			auto enabledDifference = WallPath::EnabledDifferenceDefaultValue();
			auto turnProfit = Array<int64>(turnCount + 1);

			auto distanceFromOpponentAgnet = [&](BoardPos pos) -> int32 {
				int32 res = 1001001;
				for (auto agent : opponentAgents) {
					res = std::min(res, std::abs(agent.pos.r - pos.r) + std::abs(agent.pos.c - pos.c));
				}
				return res;
				};

			for (int32 r = 0; r < board->getHeight(); r++) for (int32 c = 0; c < board->getWidth(); c++) {
				auto boardPos = BoardPos(r, c);
				int64 tProfit = 1000;
				int64 wProfit = -50;
				Array<int64> vProfit(turnCount + 1);
				auto mass = board->getMass(boardPos);
				if (mass.hasWallOf(opponentColor)) {
					tProfit = 500;
					wProfit = 1000;
				}
				if (mass.hasWallOf(myColor)) {
					tProfit = std::min(tProfit, (int64)-100);
					wProfit = std::min(wProfit, (int64)-100);
				}
				int32 dist = distanceFromOpponentAgnet(boardPos);
				if (dist == 0) {
					for (int32 t = 0; t <= turnCount; t++) vProfit[t] = -1001001001001;
				}

				territoryProfit[boardPos.asPoint()] = tProfit;
				wallProfit[boardPos.asPoint()] = wProfit;
				for (int32 t = 0; t <= turnCount; t++) visitingProfit[t][boardPos.asPoint()] = vProfit[t];
			};

			// ----------------------------------------------
			//   サブルーチン

			auto gridWalking = std::make_shared<GridWalking>(state, visitingProfit);
			auto wallPathA = std::make_shared<WallPath>(state, territoryProfit, wallProfit, enabledDifference, false);
			auto wallPathB = std::make_shared<WallPath>(state, territoryProfit, wallProfit, enabledDifference, true);
			auto constructWallPath = ConstructWallPath(gridWalking, { wallPathA, wallPathB });

			auto wallPaths = myAgents.map([&](Agent agent) { return constructWallPath.solve(agent, turnCount, turnProfit); });


			// ----------------------------------------------

			static const int64 NegativeInf = -1001001001001;
			struct SearchNode {
				int64 profit = NegativeInf;
				ShortenMoveSet firstMoves;
			};
			auto maxProfitCycle = Array<SearchNode>((size_t)1 << myAgents.size());

			for (int32 maxAgentIndex = 0; maxAgentIndex < (int32)myAgents.size(); maxAgentIndex++) {
				int32 searchSize = constructWallPath.getNumberOfWallPositions();

				auto canStart = Array<int32>(searchSize, 0);
				for (auto& a : wallPaths[maxAgentIndex]) canStart[a.from] = 1;

				for (int32 s = 0; s < (int32)canStart.size(); s++) {
					auto dp = Array<Array<SearchNode>>((size_t)1 << maxAgentIndex, Array<SearchNode>(searchSize));
					for (auto& a : wallPaths[maxAgentIndex]) if (a.from == s) if (dp[0][a.to].profit < a.profit) {
						dp[0][a.to].profit = a.profit;
						dp[0][a.to].firstMoves.setAt(maxAgentIndex, ShortenMove::Encode(a.firstMove));
					}
					for (int32 j = 0; j < (int32)dp.size(); j++) {
						maxProfitCycle[j + (1 << maxAgentIndex)] = *std::max_element(dp[j].begin(), dp[j].end(),
							[](const SearchNode& l, const SearchNode& r) { return l.profit < r.profit; });
						for (int32 ag = 0; ag < maxAgentIndex; ag++) if (!(j & (1 << ag))) {
							for (auto& a : wallPaths[ag]) {
								auto& tov = dp[j + (1 << ag)][a.to];
								int64 nxprofit = dp[j][a.from].profit + a.profit;
								if (tov.profit < nxprofit) {
									tov.profit = nxprofit;
									tov.firstMoves = dp[j][a.from].firstMoves;
									tov.firstMoves.setAt(ag, ShortenMove::Encode(a.firstMove));
								}
							}
						}
					}
				}

			}

			auto maxProfitPlan = maxProfitCycle;
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

			for (int32 i = 0; i < myAgents.size(); i++) {
				auto agentMove = ShortenMove::Decode(maxProfitPlan.back().firstMoves.getAt(i), myAgents[i]);
				result.insert(agentMove);
			}

			return result;


			/*

			// 他の職人との距離が小さい場合にペナルティをつける
			// 以下、パラメータ
			double visitingProfitCoeff_eOpponent = -10000.0;
			double visitingProfitCoeff_eAlly = -5000.0;
			// 以下、計算
			Array<Grid<int64>> visitingProfit(turnCount + 1);
			auto myAgents = state->getAgents(myColor);
			auto oppositeAgents = state->getAgents(state->OpponentOf(myColor));
			// 各ターンの各マスについて処理
			for (auto& table : visitingProfit) table.assign(boardSize, 0);
			for (int32 t = 1; t <= turnCount; t++) for (int32 r = 0; r < boardSize.y; r++) for (int32 c = 0; c < boardSize.x; c++) {
				auto position = BoardPos(r, c);
				auto posPoint = position.asPoint();
				// 相手の職人
				for (auto oppositeAgent : oppositeAgents) {
					auto posDiff = oppositeAgent.pos.asPoint() - posPoint;
					int32 cDistInt = Max(Abs(posDiff.x), Abs(posDiff.y));
					if (cDistInt > t) continue;
					//double cDist = (double)cDistInts;
					double eDist = Sqrt((double)((int64)posDiff.x * posDiff.x + (int64)posDiff.y * posDiff.y));
					double sc = 1.0 / (1.0 + eDist / t);
					if ((int64)(sc * visitingProfitCoeff_eOpponent) > 0) {
						int a = 0;
					}
					visitingProfit[t][posPoint] += (int64)(sc * visitingProfitCoeff_eOpponent);
				}
				// 味方の職人
				for (auto myAgent : myAgents) {
					auto posDiff = myAgent.pos.asPoint() - posPoint;
					int32 cDistInt = Max(Abs(posDiff.x), Abs(posDiff.y));
					if (cDistInt > t) continue;
					//double cDist = (double)cDistInts;
					double eDist = Sqrt((double)((int64)posDiff.x * posDiff.x + (int64)posDiff.y * posDiff.y));
					double sc = 1.0 / (1.0 + eDist / t);
					visitingProfit[t][posPoint] += (int64)(sc * visitingProfitCoeff_eAlly);
				}
			}

			auto gridWalking = GridWalking(state, visitingProfit);
			Array<GridWalking::Answer> shortpathAnswers(myAgents.size());
			for (auto myAgent : myAgents) {
				shortpathAnswers[myAgent.marker.index] = gridWalking.solve(myAgent, turnCount);
			}

			auto result = TurnInstruction(state, myColor);
			// 次の一手を計算

			for (auto myAgent : myAgents) {
				int32 agentIndex = myAgent.marker.index;
				auto& sps = shortpathAnswers[agentIndex].shortPath.back();
				if (sps.empty()) continue;
				size_t id = 0;
				for (size_t i = 0; i < sps.size(); i++) {
					if (sps[id].profit < sps[i].profit) id = i;
				}
				result.insert(sps[id].firstMove);
			}

			return result;
			*/


		}

		String MainSolution::name() { return U"戦略"; }

	}


} // namespace Procon34
