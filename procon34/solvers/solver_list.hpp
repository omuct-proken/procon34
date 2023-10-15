#pragma once
#include "../stdafx.h"
#include "../game_simulator.hpp"

namespace Procon34 {


	class SolverInterface {
	public:

		virtual TurnInstruction operator()(BoxPtr<const GameState> state) = 0;

		virtual String name() { return U"unnamed"; }

		static Array<std::shared_ptr<SolverInterface>> ListOfSolvers();

	};


	class SolverWrapper {
	public:

		SolverWrapper(std::shared_ptr<SolverInterface>);

		TurnInstruction operator()(BoxPtr<const GameState> state);

	private:
		std::shared_ptr<SolverInterface> solver;
	};


} // namespace Procon34
