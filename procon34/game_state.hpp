#pragma once
#include "stdafx.h"
#include "game_util.hpp"
#include "game_agent.hpp"
#include "game_board.hpp"
#include "game_instructions.hpp"
#include "request.hpp"

namespace Procon34 {


	// ----------------------------------------
	//
	//  ゲームの初期状態の情報。
	//
	struct GameInitialState {

		int32 boardWidth;
		int32 boardHeight;
		Grid<MassBiome> biomeGrid;
		EachPlayer<Array<BoardPos>> agentPos;
		int32 castleCoefficient;
		int32 teritorryCoefficient;
		int32 wallCoefficient;
		int32 turnCount;
		int32 turnTimeLimitInMiliseconds;
		PlayerColor firstToMove = PlayerColor::Red;

		// 募集要項に書いてある制約

		static constexpr int32 MinWidth = 11;
		static constexpr int32 MaxWidth = 25;
		static constexpr int32 MinHeight = 11;
		static constexpr int32 MaxHeight = 25;

		static constexpr int32 MinAgentCount = 2;
		static constexpr int32 MaxAgentCount = 6;

		static constexpr int32 MinCastleCoeff = 1;
		static constexpr int32 MaxCastleCoeff = 100;
		static constexpr int32 MinTeritorryCoeff = 1;
		static constexpr int32 MaxTeritorryCoeff = 100;
		static constexpr int32 MinWallCoeff = 1;
		static constexpr int32 MaxWallCoeff = 100;

		// 各パラメータをランダムに設定
		GameInitialState();

		// 募集要項に書いてある制約を満たせば正常に返り、 none が返される。
		// returns error messages (one line string for each)
		Optional<Array<String>> verify(bool doThrow) const;

		JSON toJson() const;
		static GameInitialState FromJson(JSON json);
	};



	//
	// 試合のある場面のデータを持ち、
	// 試合の進行を管理する
	//
	class GameState : public std::enable_shared_from_this<GameState> {
	public:

		// ゲームが終了したか？
		bool isOver() const;

		// 今誰のターンか
		PlayerColor whosTurn() const;

		// 現在のターンの番号（これまでにいくつのターンが終了したか）
		int32 getTurnIndex() const;

		// 試合全体のターン数
		int32 getAllTurnNumber() const;

		BoxPtr<GameBoard> getBoard() const noexcept;

		Array<Agent> getAgents(PlayerColor color) const;
		Array<Agent> getRedAgents() const;
		Array<Agent> getBlueAgents() const;
		Array<Agent> getAllAgents() const;

		// 成功すればターンが 1 つ進み、 true が返る。
		bool makeMove(PlayerColor player, const TurnInstruction& newInsts);

		// 指定した陣営の点数全体
		int64 getScore(PlayerColor player) const;

		// マス pos は陣営 color の陣地か？
		bool isAreaOf(PlayerColor color, BoardPos pos) const;

		// マス pos は陣営 color の閉鎖された陣地か？
		bool isClosedAreaOf(PlayerColor color, BoardPos pos) const;

		// 保存された初期状態を取得
		BoxPtr<const GameInitialState> getInitialState() const;

		// 初期状態から構築
		static BoxPtr<GameState> FromInitialState(const GameInitialState& initialState);

		static PlayerColor OpponentOf(PlayerColor player) { return player == PlayerColor::Red ? PlayerColor::Blue : PlayerColor::Red; }

		static std::array<PlayerColor, 2> AllPlayers() { return { PlayerColor::Red, PlayerColor::Blue }; }

		// 試合状態取得API の Response
		static BoxPtr<GameState> FromJson(const JSON& json, const MatchOverview& mov);

	private:
		BoxPtr<GameBoard> m_board;
		EachPlayer<Array<Agent>> m_agents;
		BoxPtr<const GameInitialState> m_initialState;

		EachPlayer<Grid<int32>> m_closedArea;
		EachPlayer<Grid<int32>> m_area;

		EachPlayer<int64> m_score;

		PlayerColor m_playerOfTurn = PlayerColor::Red;
		int32 m_turnId; // 0 at initial

		GameState();

		void initAreas();

		void recalcClosedAreas();

		void recalcOpenAreas();

		// 盤面の状態から陣地を再計算、従って得点も再計算する。
		void recalcScores();

		void changePositionOfAnAgent(AgentMarker marker, BoardPos newPos);

	};

}
