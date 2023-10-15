#pragma once

#include "game_agent.hpp"


namespace Procon34 {


	inline AgentMove::AgentMove()
		: agent(none)
		, type(AgentMoveType::move)
		, direction(none) {}

	inline AgentMove::AgentMove(Agent _agent, AgentMoveType _type, Optional<MoveDirection> _direction)
		: agent(_agent)
		, type(_type)
		, direction(_direction)
	{}

	inline AgentMove AgentMove::GetNull() {
		return AgentMove();
	}

	inline AgentMove AgentMove::GetStay(Agent agent) {
		return AgentMove(agent, AgentMoveType::stay, none);
	}

	inline AgentMove AgentMove::GetMove(Agent agent, MoveDirection direction) {
		return AgentMove(agent, AgentMoveType::move, direction);
	}

	inline AgentMove AgentMove::GetConstruct(Agent agent, MoveDirection direction) {
		if (!direction.is4Direction()) throw Error(U"Invalid argument at AgentMove::GetConstruct");
		return AgentMove(agent, AgentMoveType::construct, direction);
	}

	inline AgentMove AgentMove::GetDestroy(Agent agent, MoveDirection direction) {
		if (!direction.is4Direction()) throw Error(U"Invalid argument at AgentMove::GetDestroy");
		return AgentMove(agent, AgentMoveType::destroy, direction);
	}

	inline bool AgentMove::isStay() const { return type == AgentMoveType::stay; }
	inline bool AgentMove::isMove() const { return type == AgentMoveType::move; }
	inline bool AgentMove::isConstruct() const { return type == AgentMoveType::construct; }
	inline bool AgentMove::isDestroy() const { return type == AgentMoveType::destroy; }

	inline AgentMove::Stay AgentMove::asStay() const {
		if (!isStay()) throw Error(U"AgentMove::asStay failed");
		if (!agent.has_value()) throw Error(U"AgentMove::asStay failed");
		return Stay{ agent.value() };
	}

	inline AgentMove::Move AgentMove::asMove() const {
		if (!isMove()) throw Error(U"AgentMove::asMove failed");
		if (!agent.has_value()) throw Error(U"AgentMove::asMove failed");
		if (!direction.has_value()) throw Error(U"AgentMove::asMove failed");
		return Move{ agent.value(), direction.value() };
	}

	inline AgentMove::Construct AgentMove::asConstruct() const {
		if (!isConstruct()) throw Error(U"AgentMove::asConstruct failed");
		if (!agent.has_value()) throw Error(U"AgentMove::asConstruct failed");
		if (!direction.has_value()) throw Error(U"AgentMove::asConstruct failed");
		return Construct{ agent.value(), direction.value() };
	}

	inline AgentMove::Destroy AgentMove::asDestroy() const {
		if (!isDestroy()) throw Error(U"AgentMove::asDestroy failed");
		if (!agent.has_value()) throw Error(U"AgentMove::asDestroy failed");
		if (!direction.has_value()) throw Error(U"AgentMove::asDestroy failed");
		return Destroy{ agent.value(), direction.value() };
	}

	inline Optional<Agent> AgentMove::getAgent() const {
		return agent;
	}

}
