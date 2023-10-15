#include "../stdafx.h"
#include "solver_list.hpp"

namespace Procon34 {


	namespace Solvers {


		class MainSolution : public SolverInterface {

			// 盤面の状態から指示を作る
			TurnInstruction operator()(BoxPtr<const GameState> state);

			String name();

		};

	}


} // namespace Procon34
