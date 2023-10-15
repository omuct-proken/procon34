#include "game_state.hpp"
#include "request.hpp"
#include "dsu_fast.hpp"

namespace Procon34 {

	GameInitialState::GameInitialState() {
		uint64 seed = Time::GetSecSinceEpoch();
		auto rng = SmallRNG(seed);
		using Uni = UniformIntDistribution<int32>;
		boardWidth = Uni(MinWidth, MaxWidth)(rng);
		boardHeight = Uni(MinHeight, MaxHeight)(rng);
		biomeGrid.resize(boardWidth, boardHeight, MassBiome::Normal);
		int32 agentCount = Uni(MinAgentCount, MaxAgentCount)(rng);

		castleCoefficient = Uni(MinCastleCoeff, MaxCastleCoeff)(rng);
		teritorryCoefficient = Uni(MinTeritorryCoeff, MaxTeritorryCoeff)(rng);
		wallCoefficient = Uni(MinWallCoeff, MaxWallCoeff)(rng);

		int32 pondCount = 10;
		int32 castleCount = 3;
		int32 chooseCnt = agentCount * 2 + pondCount + castleCount;
		Array<int32> choose = Range(0, boardWidth * boardHeight - 1).asArray().choice(chooseCnt, rng).shuffled(rng);
		auto int32ToPos = [&](int32 x) -> BoardPos { return BoardPos(x / boardWidth, x % boardWidth); };
		for (PlayerColor player : { PlayerColor::Red, PlayerColor::Blue }) {
			for (int32 i = 0; i < agentCount; i++) {
				agentPos[player].push_back(int32ToPos(choose.back()));
				choose.pop_back();
			}
		}
		for (int32 i = 0; i < pondCount; i++) {
			biomeGrid[int32ToPos(choose.back()).asPoint()] = MassBiome::Pond;
			choose.pop_back();
		}
		for (int32 i = 0; i < castleCount; i++) {
			biomeGrid[int32ToPos(choose.back()).asPoint()] = MassBiome::Castle;
			choose.pop_back();
		}

		turnCount = Uni(3, 20)(rng) * 10;
		turnTimeLimitInMiliseconds = Uni(3, 20)(rng) * 1000;
	}


	JSON GameInitialState::toJson() const{

		auto PlayerColorToString = [](PlayerColor c) -> const char32* {
			return c == PlayerColor::Red ? U"red" : U"blue";
		};

		JSON res;
		res[U"boardWidth"] = boardWidth;
		res[U"boardHeight"] = boardHeight;
		for (size_t y = 0; y < biomeGrid.height(); y++) {
			String buf;
			for (size_t x = 0; x < biomeGrid.width(); x++) {
				char32 ch = U'*';
				switch (biomeGrid[y][x]) {
				case MassBiome::Pond: ch = U'0'; break;
				case MassBiome::Castle: ch = U'1'; break;
				case MassBiome::Normal: ch = U'2'; break;
				}
				buf.push_back(ch);
			}
			res[U"biomeGrid"][y] = buf;
		}
		for (auto c : GameState::AllPlayers()) {
			JSON res2;
			for (size_t i = 0; i < agentPos[c].size(); i++) {
				res2[i][0] = agentPos[c][i].r;
				res2[i][1] = agentPos[c][i].c;
			}
			res[U"agentPos"][PlayerColorToString(c)] = res2;
		}
		res[U"castleCoefficient"] = castleCoefficient;
		res[U"teritorryCoefficient"] = teritorryCoefficient;
		res[U"wallCoefficient"] = wallCoefficient;
		res[U"turnCount"] = turnCount;
		res[U"turnTimeLimitInMiliseconds"] = turnTimeLimitInMiliseconds;
		res[U"firstToMove"] = PlayerColorToString(firstToMove);

		return res;
	}

	GameInitialState GameInitialState::FromJson(JSON json) {
		auto PlayerColorToString = [](PlayerColor c) -> const char32* {
			return c == PlayerColor::Red ? U"red" : U"blue";
		};
		auto PlayerColorFromJSON = [](JSON c) -> PlayerColor {
			return c.getString() == U"red" ? PlayerColor::Red : PlayerColor::Blue;
		};

		GameInitialState res;

		res.boardWidth = json[U"boardWidth"].get<int32>();
		res.boardHeight = json[U"boardHeight"].get<int32>();

		res.biomeGrid.assign(Size(res.boardWidth, res.boardHeight), MassBiome::Normal);
		for (int32 y = 0; y < res.boardHeight; y++) {
			// if(json[U"biomeGrid"].size() != res.boardHeight) throw Error(U"GameInitialState.biomeGrid : 要素数が異なります");
			auto s = json[U"biomeGrid"][y].getString();
			if ((int)s.size() != res.boardWidth) throw Error(U"GameInitialState.biomeGrid[{}] : 文字列の長さが異なります。"_fmt(y));
			for (int32 x = 0; x < res.boardWidth; x++) {
				MassBiome biomebuf = MassBiome::Normal;
				switch (s[x]) {
				case U'0': case U'p': case U'P': biomebuf = MassBiome::Pond; break;
				case U'1': case U'c': case U'C': biomebuf = MassBiome::Castle; break;
				}
				res.biomeGrid[y][x] = biomebuf;
			}
		}

		for (auto c : GameState::AllPlayers()) {
			res.agentPos[c].clear();
			auto jsonc = json[U"agentPos"][PlayerColorToString(c)];
			for (JSONItem jsonci : jsonc) {
				int32 posr = jsonci.value[0].get<int32>();
				int32 posc = jsonci.value[1].get<int32>();
				res.agentPos[c].push_back(BoardPos(posr, posc));
			}
		}

		res.castleCoefficient = json[U"castleCoefficient"].get<int32>();
		res.teritorryCoefficient = json[U"teritorryCoefficient"].get<int32>();
		res.wallCoefficient = json[U"wallCoefficient"].get<int32>();
		res.turnCount = json[U"turnCount"].get<int32>();
		res.turnTimeLimitInMiliseconds = json[U"turnTimeLimitInMiliseconds"].get<int32>();
		res.firstToMove = PlayerColorFromJSON(json[U"firstToMove"]);

		return res;
	}


	// returns error messages (one line string for each)
	Optional<Array<String>> GameInitialState::verify(bool doThrow) const {
		Array<String> errMsg;

		auto makeErrMsg = [&]() -> String {
			String res;
			for (auto& s : errMsg) {
				res += U" - ";
				res += s;
				res += U"\n";
			}
			return res;
		};

		auto isOnBoard = [&](BoardPos pos) -> bool {
			return 0 <= pos.r && pos.r < boardHeight && 0 <= pos.c && pos.c < boardWidth;
		};

		auto colorStr = [&](PlayerColor c) -> const char32_t* {
			return c == PlayerColor::Red ? U"Red" : U"Blue";
		};

		if (boardWidth < MinWidth) {
			errMsg.push_back(U"boardWidth < {} (boardWidth = {})"_fmt(MinWidth, boardWidth));
		}
		if (MaxWidth < boardWidth) {
			errMsg.push_back(U"{} < boardWidth (boardWidth = {})"_fmt(MaxWidth, boardWidth));
		}
		if (boardHeight < MinHeight) {
			errMsg.push_back(U"boardHeight < {} (boardHeight = {})"_fmt(MinHeight, boardHeight));
		}
		if (MaxHeight < boardHeight) {
			errMsg.push_back(U"{} < boardHeight (boardHeight = {})"_fmt(MaxHeight, boardHeight));
		}

		auto boardSize = Size(boardWidth, boardHeight);

		if (biomeGrid.size() != boardSize) {
			errMsg.push_back(U"biomeGrid.size() != boardSize (biomeGrid.size() = {}, boardSize = {})"_fmt(biomeGrid.size(), boardSize));
		}

		for (PlayerColor player : { PlayerColor::Red, PlayerColor::Blue }) {

			if (agentPos[player].size() < MinAgentCount) {
				String x = U"agnetPos[{}]"_fmt(colorStr(player));
				errMsg.push_back(U"{}.size() < {} ({}.size() = {})"_fmt(x, MinAgentCount, x, agentPos[player].size()));
			}
			if (MaxAgentCount < agentPos[player].size()) {
				String x = U"agnetPos[{}]"_fmt(colorStr(player));
				errMsg.push_back(U"{} < {}.size() ({}.size() = {})"_fmt(MaxAgentCount, x, x, agentPos[player].size()));
			}

			agentPos[player].each_index([&](size_t idx, BoardPos pos) -> void {
				if (!isOnBoard(pos)) {
					String x = U"agnetPos[{}][{}]"_fmt(colorStr(player), idx);
					errMsg.push_back(U"{} is out of range ({} = {}, boardSize = {})"_fmt(x, x, pos, boardSize));
				}
			});
		}

		if (castleCoefficient < MinCastleCoeff) {
			errMsg.push_back(U"castleCoefficient < {} (castleCoefficient = {})"_fmt(MinCastleCoeff, castleCoefficient));
		}
		if (MaxCastleCoeff < castleCoefficient) {
			errMsg.push_back(U"{} < castleCoefficient (castleCoefficient = {})"_fmt(MaxCastleCoeff, castleCoefficient));
		}
		if (teritorryCoefficient < MinTeritorryCoeff) {
			errMsg.push_back(U"teritorryCoefficient < {} (teritorryCoefficient = {})"_fmt(MinTeritorryCoeff, teritorryCoefficient));
		}
		if (MaxTeritorryCoeff < teritorryCoefficient) {
			errMsg.push_back(U"{} < teritorryCoefficient (teritorryCoefficient = {})"_fmt(MaxTeritorryCoeff, teritorryCoefficient));
		}
		if (wallCoefficient < MinWallCoeff) {
			errMsg.push_back(U"wallCoefficient < {} (wallCoefficient = {})"_fmt(MinWallCoeff, wallCoefficient));
		}
		if (MaxWallCoeff < wallCoefficient) {
			errMsg.push_back(U"{} < wallCoefficient (wallCoefficient = {})"_fmt(MaxWallCoeff, wallCoefficient));
		}

		if (!errMsg.empty() && doThrow) throw Error(makeErrMsg());

		if (errMsg.empty()) return none; else return errMsg;
	}


	GameState::GameState()
		: m_turnId(0)
		, m_score(0, 0)
	{}

	void GameState::initAreas() {
		auto boardSize = m_board->getSize();
		m_closedArea.assign(Grid<int32>(boardSize, 0), Grid<int32>(boardSize, 0));
		m_area.assign(Grid<int32>(boardSize, 0), Grid<int32>(boardSize, 0));
	}

	// 閉鎖された陣地の再計算。
	void GameState::recalcClosedAreas() {
		int32 h = m_board->getHeight();
		int32 w = m_board->getWidth();

		int32 dsusize = (h + 2) * (w + 2); // 外周に 1 マス足す

		auto isOwnWall = [&](PlayerColor player, BoardPos pos) {
			return m_board->isOnBoard(pos)
				&& m_board->operator[](pos).wall.has_value()
				&& m_board->operator[](pos).wall->color == player;
		};

		// 外周 1 マスを考慮して dsu の番号を計算
		auto dsuIdx = [w](int r, int c) { return (r + 1) * (w + 2) + c + 1; };

		for (auto player : AllPlayers()) {

			// Union-Find で閉鎖された陣地を計算する。
			auto dsu = nachia::DsuFast(dsusize);

			// 隣接するマスが両方空いていれば、結合する。
			for (int32 r = 0; r <= h; r++) {
				for (int32 c = -1; c <= w; c++) {
					if (!isOwnWall(player, BoardPos(r, c)) && !isOwnWall(player, BoardPos(r - 1, c))) {
						dsu.merge(dsuIdx(r - 1, c), dsuIdx(r, c));
					}
				}
			}
			for (int32 r = -1; r <= h; r++) {
				for (int32 c = 0; c <= w; c++) {
					if (!isOwnWall(player, BoardPos(r, c)) && !isOwnWall(player, BoardPos(r, c - 1))) {
						dsu.merge(dsuIdx(r, c - 1), dsuIdx(r, c));
					}
				}
			}

			// そのマスに壁がなくて、外周と結合されていなければ、閉鎖された陣地である。
			for (int32 r = 0; r < h; r++) {
				for (int32 c = 0; c < w; c++) {
					auto pos = BoardPos(r, c);
					m_closedArea[player][pos.asPoint()] = (!isOwnWall(player, pos) && !dsu.same(0, dsuIdx(r, c)) ? 1 : 0);
				}
			}
		}
	}

	// 閉鎖された陣地が計算された後の、陣地の再計算。
	void GameState::recalcOpenAreas() {
		int32 h = m_board->getHeight();
		int32 w = m_board->getWidth();

		for (int32 r = 0; r < h; r++) {
			for (int32 c = 0; c < w; c++) {
				for (auto player : AllPlayers()) {
					auto pos = BoardPos(r, c);
					if (m_closedArea[OpponentOf(player)][pos.asPoint()]) m_area[player][pos.asPoint()] = 0;
					if ((*m_board)[pos].wall.has_value()) m_area[player][pos.asPoint()] = 0;
					if (m_closedArea[player][pos.asPoint()]) m_area[player][pos.asPoint()] = 1;
				}
			}
		}
	}

	void GameState::recalcScores() {
		m_score.assign(0, 0);

		int32 h = m_board->getHeight();
		int32 w = m_board->getWidth();
		for (int32 r = 0; r < h; r++) {
			for (int32 c = 0; c < w; c++) {
				auto pos = BoardPos(r, c);
				auto wall = (*m_board)[pos].wall;
				if (wall.has_value()) {
					(m_score[wall->color]) += m_initialState->wallCoefficient;
				}
				for(auto player : AllPlayers()){
					if (m_area[player][pos.asPoint()]) {
						if ((*m_board)[pos].biome == MassBiome::Castle) {
							m_score[player] += m_initialState->castleCoefficient;
						}
						m_score[player] += m_initialState->teritorryCoefficient;
					}
				}
			}
		}
	}

	bool GameState::isOver() const { return m_turnId >= m_initialState->turnCount; }
	PlayerColor GameState::whosTurn() const { return m_playerOfTurn; }
	int32 GameState::getTurnIndex() const { return m_turnId; }
	int32 GameState::getAllTurnNumber() const { return m_initialState->turnCount; }

	BoxPtr<GameBoard> GameState::getBoard() const noexcept { return m_board; }

	Array<Agent> GameState::getAgents(PlayerColor color) const { return m_agents[color]; }
	Array<Agent> GameState::getRedAgents() const { return getAgents(PlayerColor::Red); }
	Array<Agent> GameState::getBlueAgents() const { return getAgents(PlayerColor::Blue); }
	Array<Agent> GameState::getAllAgents() const {
		Array<Agent> res;
		for (auto& a : m_agents[PlayerColor::Red]) res.push_back(a);
		for (auto& a : m_agents[PlayerColor::Blue]) res.push_back(a);
		return res;
	}

	bool GameState::makeMove(PlayerColor player, const TurnInstruction& newInsts) {
		if (isOver()) return false;

		Array<AgentMove> insts = newInsts.m_data;

		auto& myAgents = m_agents[player];

		if (insts.size() != myAgents.size()) {
			throw Error(U"GameState::makeMove failed (numAgents != move.size(), numAgents = {} , insts.size() = {}"_fmt(myAgents.size(), insts.size()));
		}

		// 解体を実行する。
		// ただし、壁の無いマス、または盤外に向けて解体はできない。
		for (auto& inst : insts) if (inst.isDestroy()) {
			auto instd = inst.asDestroy();
			auto pos = instd.agent.pos.movedAlong(instd.dir);
			if (
				!m_board->isOnBoard(pos)
				|| !(*m_board)[pos].wall.has_value()
			) {
				inst = AgentMove::GetStay(instd.agent); continue;
			}

			(*m_board)[pos].wall = none;
		}

		// 建築を実行する。
		// ただし、城のマス、または盤外、または壁がすでにあるマスに向けて建築はできない。
		// 相手の職人がいるマスにも建築できない。
		for (auto& inst : insts) if (inst.isConstruct()) {
			auto instc = inst.asConstruct();
			auto pos = instc.agent.pos.movedAlong(instc.dir);
			if (
				!m_board->isOnBoard(pos)
				|| (*m_board)[pos].biome == MassBiome::Castle
				|| (*m_board)[pos].wall.has_value()
				|| ((*m_board)[pos].hasAgent() && (*m_board)[pos].agent->color != player)
			) {
				inst = AgentMove::GetStay(instc.agent); continue;
			}

			(*m_board)[pos].wall = WallData{ player };
		}


		// 移動が可能かどうか判定するためのマークの個数
		Grid<int> reservedMove(getBoard()->getSize(), 0);

		// 移動先をマークする。
		for (auto& inst : insts) if (inst.isMove()) {
			auto instm = inst.asMove();
			auto newPos = instm.agent.pos.movedAlong(instm.dir);
			if (m_board->isOnBoard(newPos)) {
				reservedMove[newPos.asPoint()] += 1;
			}
		}

		// 盤外、池、他の職人、自分のもの以外の壁があるマスへの移動はキャンセルされる。
		for (auto& inst : insts) if (inst.isMove()) {
			auto instm = inst.asMove();
			auto newPos = instm.agent.pos.movedAlong(instm.dir);
			if (
				!m_board->isOnBoard(newPos)
				|| ((*m_board)[newPos].biome == MassBiome::Pond)
				|| ((*m_board)[newPos].agent.has_value())
				|| ((*m_board)[newPos].wall.value_or(WallData{ player }).color != player)
			) {
				inst = AgentMove::GetStay(instm.agent); continue;
			}
		}

		// 2 個以上マークがあるところに移動はできない。
		for (auto& inst : insts) if (inst.isMove()) {
			auto instm = inst.asMove();
			auto newPos = instm.agent.pos.movedAlong(instm.dir);
			if (reservedMove[newPos.asPoint()] >= 2) {
				inst = AgentMove::GetStay(instm.agent);
			}
		}

		// 移動を実行する。
		for (auto& inst : insts) if (inst.isMove()) {
			auto instm = inst.asMove();
			auto prevPos = instm.agent.pos;
			auto newPos = prevPos.movedAlong(instm.dir);
			changePositionOfAnAgent(instm.agent.marker, newPos);
		}

		// 得点の再計算
		recalcClosedAreas();
		recalcOpenAreas();
		recalcScores();

		// ターン番号を進める。
		m_turnId++;
		m_playerOfTurn = OpponentOf(m_playerOfTurn);

		return true;
	}


	BoxPtr<const GameInitialState> GameState::getInitialState() const {
		return m_initialState;
	}

	int64 GameState::getScore(PlayerColor player) const {
		return m_score[player];
	}

	bool GameState::isAreaOf(PlayerColor color, BoardPos pos) const {
		return m_area[color][pos.asPoint()];
	}

	bool GameState::isClosedAreaOf(PlayerColor color, BoardPos pos) const {
		return m_closedArea[color][pos.asPoint()];
	}

	BoxPtr<GameState> GameState::FromInitialState(const GameInitialState& initialState) {

		class ConstructionHelper : public GameState {
		public:
			ConstructionHelper() : GameState() {}
		};

		auto board = std::make_shared<GameBoard>(initialState.boardWidth, initialState.boardHeight);
		for (int32 r = 0; r < board->getHeight(); r++) {
			for (int32 c = 0; c < board->getWidth(); c++) {
				auto pos = BoardPos(r, c);
				auto& mass = (*board)[pos];
				mass.biome = initialState.biomeGrid[pos.asPoint()];
			}
		}

		auto agents = EachPlayer<Array<Agent>>();

		for (auto player : { PlayerColor::Red, PlayerColor::Blue }) {
			initialState.agentPos[player].each_index(
				[&](size_t idx, BoardPos pos) -> void {
					auto agent = Agent{ .marker = AgentMarker{.color = player, .index = (int32)idx }, .pos = pos };
					(*board)[pos].agent = agent.marker;
					agents[player].push_back(agent);
				}
			);
		}

		BoxPtr<GameState> res = std::make_shared<ConstructionHelper>();
		res->m_board = board;
		res->m_agents = std::move(agents);
		res->m_initialState = std::make_shared<const GameInitialState>(initialState); // copying
		res->m_playerOfTurn = initialState.firstToMove;

		res->initAreas();

		return res;
	}

	// 指定した Agent の場所を変更する。
	// したがって board の情報も変更する。
	void GameState::changePositionOfAnAgent(AgentMarker marker, BoardPos newPos) {
		auto prevPos = m_agents[marker.color][marker.index].pos;
		(*m_board)[prevPos].agent = none;
		(*m_board)[newPos].agent = marker;
		m_agents[marker.color][marker.index].pos = newPos;
	}

	// 試合状態取得API の Response ([GET] /matches/{id})
	// https://www.procon.gr.jp/wp-content/uploads/2023/07/spec.html#operation/GetMatch
	// JSON の型みたいなのがないから不安になってくる

	BoxPtr<GameState> GameState::FromJson(const JSON& json, const MatchOverview& mov) {
		class ConstructionHelper : public GameState {
		public:
			ConstructionHelper() : GameState() {}
		};

		// GameInitialstate の構築
		auto initialstate = std::make_shared<GameInitialState>();
		initialstate->boardWidth = mov.boardWidth;
		initialstate->boardHeight = mov.boardHeight;
		initialstate->castleCoefficient = mov.bonusCastle;
		initialstate->teritorryCoefficient = mov.bonusTerritory;
		initialstate->wallCoefficient = mov.bonusWall;
		initialstate->turnCount = mov.turns;
		initialstate->turnTimeLimitInMiliseconds = mov.turnSeconds * 1000;
		initialstate->firstToMove = mov.first ? PlayerColor::Red : PlayerColor::Blue;
		initialstate->biomeGrid.assign(Size(mov.boardWidth, mov.boardHeight), MassBiome::Normal);
		initialstate->agentPos[PlayerColor::Red] = mov.myAgentPos;
		initialstate->agentPos[PlayerColor::Blue] = mov.opponentAgentPos;

		JSONArrayView biomes = json[U"board"][U"structures"].arrayView();
		for (int32 r = 0; r < mov.boardHeight; r++) {
			for (int32 c = 0; c < mov.boardWidth; c++) {
				auto pos = BoardPos(r, c);

				int32 biome = biomes[r][c].get<int32>();

				if (biome == 0) {
					initialstate->biomeGrid[pos.asPoint()] = MassBiome::Normal;
				}
				else if (biome == 1) {
					initialstate->biomeGrid[pos.asPoint()] = MassBiome::Pond;
				}
				else {
					initialstate->biomeGrid[pos.asPoint()] = MassBiome::Castle;
				}
			}
		}

		// GameBoard の構築
		auto gameboard = std::make_shared<GameBoard>(initialstate->boardWidth, initialstate->boardHeight);

		for (int32 r = 0; r < gameboard->getHeight(); r++) {
			for (int32 c = 0; c < gameboard->getWidth(); c++) {
				auto pos = BoardPos(r, c);
				auto& mass = (*gameboard)[pos];

				int32 biome = biomes[r][c].get<int32>();

				if (biome == 0) {
					mass.biome = MassBiome::Normal;
				}
				else if (biome == 1) {
					mass.biome = MassBiome::Pond;
				}
				else {
					mass.biome = MassBiome::Castle;
				}
			}
		}

		// Agents の構築
		auto agents = EachPlayer<Array<Agent>>();
		int32 agent_headcount = json[U"board"][U"mason"].get<int32>();
		JSONArrayView agents_array = json[U"board"][U"masons"].arrayView();

		agents[PlayerColor::Red].resize(agent_headcount);
		agents[PlayerColor::Blue].resize(agent_headcount);

		for (int32 r = 0; r < gameboard->getHeight(); r++) {
			for (int32 c = 0; c < gameboard->getWidth(); c++) {
				int32 agent_idx = agents_array[r][c].get<int32>();

				// 味方は正
				if (agent_idx > 0) {
					int32 destIndex = agent_idx - 1;
					auto agent = Agent{ .marker = AgentMarker{.color = PlayerColor::Red, .index = destIndex }, .pos = BoardPos(r, c) };
					agents[PlayerColor::Red][destIndex] = agent;
				}

				// 敵は負
				else if (agent_idx < 0) {
					int32 destIndex = -agent_idx - 1;
					auto agent = Agent{ .marker = AgentMarker{.color = PlayerColor::Blue, .index = destIndex }, .pos = BoardPos(r, c) };
					agents[PlayerColor::Blue][destIndex] = agent;
				}
			}
		}

		// かべを処理してない
		// WallData をみる
		// Gameboard にかくのう
		Array<Mass> mass;
		JSONArrayView walls = json[U"board"][U"walls"].arrayView();

		for (int32 r = 0; r < gameboard->getHeight(); r++) {
			for (int32 c = 0; c < gameboard->getWidth(); c++) {
				int32 wall_num = walls[r][c].get<int32>();
				auto pos = BoardPos(r, c);

				// 味方は 1
				if (wall_num == 1) {
					(*gameboard)[pos].wall = WallData{ PlayerColor::Red };
				}

				// 敵は 2
				else if (wall_num == 2) {
					(*gameboard)[pos].wall = WallData{ PlayerColor::Blue };
				}

				// なしは 0
				else {
					(*gameboard)[pos].wall = none;
				}
			}
		}

		BoxPtr<GameState> res = std::make_shared<ConstructionHelper>();

		// GameState の構築
		res->m_board = gameboard;
		res->m_agents = std::move(agents);
		res->m_initialState = initialstate;
		res->m_turnId = json[U"turn"].get<int32>();

		int32 parityTurn = (mov.first ? 1 : 0) ^ (res->m_turnId % 2);
		res->m_playerOfTurn = parityTurn ? PlayerColor::Red : PlayerColor::Blue;
		res->initAreas();

		return res;
	}
}
