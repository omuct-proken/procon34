#pragma once
#include "game_simulator.hpp"


namespace Procon34 {

	namespace ViewerNS {
		struct MatchDigest;
	}


	class MatchViewer {
	public:

		MatchViewer(MatchRecord record);

		void drawBoard(RectF rect, int turnIndex);

		void drawBoardAndInfo(RectF rect, int turnIndex, Font font);

		int32 getTurnCount() const;

	private:

		std::shared_ptr<ViewerNS::MatchDigest> m_digest;

		bool m_doDrawInstructions = false;

	};

}

