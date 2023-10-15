#pragma once
#include "../stdafx.h"
#include "../game_state.hpp"

namespace Procon34 {

	class ShortenMove {
	public:

		using Type = uint32;
		static constexpr const int32 ShiftSize = 5;

		static Type Stay();
		static Type Construct(MoveDirection);
		static Type Destroy(MoveDirection);
		static Type Move(MoveDirection);

		static Type Encode(AgentMove);
		static AgentMove Decode(Type val, Agent agent);

	};

	class ShortenMoveSet {
		uint32 m_valSet;
	public:

		ShortenMove::Type getAt(int32 index) const;
		void setAt(int32 index, ShortenMove::Type val);

	};

}

