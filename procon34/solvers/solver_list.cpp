#include "../stdafx.h"
#include "solver_list.hpp"
#include "grid_shortpath.hpp"
#include "solver_main.hpp"
#include "solver_main2.hpp"

namespace Procon34 {


	namespace Solvers {

		class AllStay : public SolverInterface {

			// 盤面の状態から指示を作る
			inline TurnInstruction operator()(BoxPtr<const GameState> state) {
				const PlayerColor turn = state->whosTurn();
				const Array<Agent> agents = state->getAgents(turn);

				// --------------------------------
				// GameState を受け取ってする処理を書く
				// --------------------------------

				TurnInstruction result(state, turn);

				return result;
			}

			String name() { return U"滞在"; }

		};

		class RandomWalk : public SolverInterface {

			SmallRNG rng;

			// 盤面の状態から指示を作る
			inline TurnInstruction operator()(BoxPtr<const GameState> state) {
				const PlayerColor turn = state->whosTurn();
				const Array<Agent> agents = state->getAgents(turn);

				// --------------------------------
				// GameState を受け取ってする処理を書く
				// --------------------------------

				TurnInstruction result(state, turn);

				for (auto agent : agents) {
					int direction = UniformIntDistribution(0, 7)(rng);
					result.insert(AgentMove::GetMove(agent, MoveDirection(direction)));
				}

				return result;
			}

			String name() { return U"ランダムウォーク"; }

		};


		class StrictRound : public SolverInterface {

			SmallRNG rng;
			Array<AgentMove> movebuf;
			int32 movePtr = 0;

			// 盤面の状態から指示を作る
			inline TurnInstruction operator()(BoxPtr<const GameState> state) {
				const PlayerColor turn = state->whosTurn();
				const Array<Agent> agents = state->getAgents(turn);

				// --------------------------------
				// GameState を受け取ってする処理を書く
				// --------------------------------

				auto boardSize = state->getBoard()->getSize();

				if (state->getTurnIndex() <= 1) {
					auto wallScore = Grid<int64>(boardSize, 20);
					auto areaScore = Grid<int64>(boardSize, 1000);
					Array<Point> acceptDiff;
					for (int32 dx = -2; dx <= 2; dx++) for (int32 dy = -2; dy <= 2; dy++) if (abs(dx) + abs(dy) <= 3) {
						acceptDiff.push_back(Point(dx, dy));
					}
					auto dijkstra = WallPath(state, areaScore, wallScore, acceptDiff);
					movebuf = dijkstra.doOneCycle(agents[0], 30);
				}

				TurnInstruction result(state, turn);

				/*
				for (auto agent : agents) {
					int direction = UniformIntDistribution(0, 7)(rng);
					result.insert(AgentMove::GetMove(agent, MoveDirection(direction)));
				}
				*/

				if (movePtr < (int32)movebuf.size()) {
					result.insert(movebuf[movePtr++]);
				}

				return result;
			}

			String name() { return U"輪っかを作る"; }

		};


		class Increasing : public SolverInterface {

			Grid<int64> placeScore;
			Array<AgentMove> movebuf;
			int32 movePtr = 0;
			bool initialized = false;

			// 盤面の状態から指示を作る
			inline TurnInstruction operator()(BoxPtr<const GameState> state) {
				const PlayerColor turn = state->whosTurn();
				const Array<Agent> agents = state->getAgents(turn);

				// --------------------------------
				// GameState を受け取ってする処理を書く
				// --------------------------------

				auto boardSize = state->getBoard()->getSize();

				if (state->getTurnIndex() <= 1) {
					auto wallScore = Grid<int64>(boardSize, 20);
					auto areaScore = Grid<int64>(boardSize, 1000);
					Array<Point> acceptDiff;
					for (int32 dx = -2; dx <= 2; dx++) for (int32 dy = -2; dy <= 2; dy++) if (abs(dx) + abs(dy) <= 3) {
						acceptDiff.push_back(Point(dx, dy));
					}
					auto dijkstra = WallPath(state, areaScore, wallScore, acceptDiff);
					movebuf = dijkstra.doOneCycle(agents[0], 30);
				}

				TurnInstruction result(state, turn);

				/*
				for (auto agent : agents) {
					int direction = UniformIntDistribution(0, 7)(rng);
					result.insert(AgentMove::GetMove(agent, MoveDirection(direction)));
				}
				*/

				if (movePtr < (int32)movebuf.size()) {
					result.insert(movebuf[movePtr++]);
				}

				return result;
			}

			String name() { return U"サブ戦略"; }

		};

	}




	Array<std::shared_ptr<SolverInterface>> SolverInterface::ListOfSolvers() {
		Array<std::shared_ptr<SolverInterface>> res;

		res.push_back(std::make_shared<Solvers::AllStay>());
		res.push_back(std::make_shared<Solvers::RandomWalk>());
		res.push_back(std::make_shared<Solvers::StrictRound>());
		res.push_back(std::make_shared<Solvers::MainSolution>());
		res.push_back(std::make_shared<Solvers::MainSolution2>());

		return res;
	}


	SolverWrapper::SolverWrapper(std::shared_ptr<SolverInterface> src) {
		if (!src) throw Error(U"SolverWrapper に nullptr が渡されました。");
		solver = src;
	}


	TurnInstruction SolverWrapper::operator()(BoxPtr<const GameState> state) {
		return solver->operator()(state);
	}


} // namespace Procon34
