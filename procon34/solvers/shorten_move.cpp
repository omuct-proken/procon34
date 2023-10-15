#pragma once
#include "shorten_move.hpp"

namespace Procon34 {

	ShortenMove::Type ShortenMove::Stay() { return 0; }
	ShortenMove::Type ShortenMove::Move(MoveDirection dir) { return 8 + dir.value(); }
	ShortenMove::Type ShortenMove::Construct(MoveDirection dir) { return 16 + dir.value(); }
	ShortenMove::Type ShortenMove::Destroy(MoveDirection dir) { return 24 + dir.value(); }

	ShortenMove::Type ShortenMove::Encode(AgentMove src) {
		if (src.isStay()) {
			return Stay();
		}
		else if (src.isMove()) {
			return Move(src.asMove().dir);
		}
		else if (src.isConstruct()) {
			return Construct(src.asConstruct().dir);
		}
		else {
			return Destroy(src.asDestroy().dir);
		}
	}

	AgentMove ShortenMove::Decode(ShortenMove::Type val, Agent agent) {
		switch (val >> 3) {
		case 0:
			return AgentMove::GetStay(agent);
		case 1:
			return AgentMove::GetMove(agent, MoveDirection(val & 7));
		case 2:
			return AgentMove::GetConstruct(agent, MoveDirection(val & 7));
		case 3:
			return AgentMove::GetDestroy(agent, MoveDirection(val & 7));
		}
		throw Error(U"WallPath::ShortenMove::output failed");
		// return AgentMove::GetStay(agent);
	}




	ShortenMove::Type ShortenMoveSet::getAt(int32 index) const {
		return (m_valSet >> (index * ShortenMove::ShiftSize)) & (((uint32)1 << ShortenMove::ShiftSize) - 1);
	}

	void ShortenMoveSet::setAt(int32 index, ShortenMove::Type val) {
		m_valSet &= ~((((uint32)1 << ShortenMove::ShiftSize) - 1) << (index * ShortenMove::ShiftSize));
		m_valSet |= (uint32)val << (index * ShortenMove::ShiftSize);
	}


}

