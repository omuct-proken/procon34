#pragma once
#include "stdafx.h"
#include "game_util.hpp"


namespace Procon34 {

	// ある試合の中で、職人を区別できる情報
	// →→ どのプレイヤーの何番の職人か？
	struct AgentMarker {
		PlayerColor color;
		int32 index;

		bool operator==(const AgentMarker& other) const { return color == other.color && index == other.index; }
		bool operator!=(const AgentMarker& other) const { return !operator==(other); }
	};

	// AgentMarker と、座標
	struct Agent {
		AgentMarker marker;
		BoardPos pos;
	};


	// 職人の移動の種類を表す
	struct AgentMove {
	public:

		enum AgentMoveType {
			stay = 0,
			move = 1,
			construct = 2,
			destroy = 3
		};

		// -----------------------------------
		//   各種操作を表す型

		struct Stay {
			Agent agent;
		};
		struct Move {
			Agent agent;
			MoveDirection dir;
		};
		struct Construct {
			Agent agent;
			MoveDirection dir;
		};
		struct Destroy {
			Agent agent;
			MoveDirection dir;
		};

		// -----------------------------------
		//   AgentMove のインスタンスを取得する。

		static AgentMove GetNull();
		static AgentMove GetStay(Agent agent);
		static AgentMove GetMove(Agent agent, MoveDirection direction);
		static AgentMove GetConstruct(Agent agent, MoveDirection direction);
		static AgentMove GetDestroy(Agent agent, MoveDirection direction);

		// -----------------------------------
		//   どの種類の操作を持っているかを確認する。

		bool isStay() const;
		bool isMove() const;
		bool isConstruct() const;
		bool isDestroy() const;

		// -----------------------------------
		//   操作の具体的な情報を取得する。
		//   間違った種類を指定すると例外を投げる。

		Stay asStay() const;
		Move asMove() const;
		Construct asConstruct() const;
		Destroy asDestroy() const;
		Optional<Agent> getAgent() const;

	private:
		Optional<Agent> agent;
		AgentMoveType type;
		Optional<MoveDirection> direction;

		AgentMove();
		AgentMove(Agent _agent, AgentMoveType _type, Optional<MoveDirection> _direction);
	};


}


#include "game_agent.ipp"
