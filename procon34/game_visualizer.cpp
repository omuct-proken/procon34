
#include "game_visualizer_buttons.hpp"
#include "game_visualizer.hpp"

#include "module_visualize/ver_01.hpp"

namespace Procon34 {


	GameVisualizer::GameVisualizer(BoxPtr<GameState> state, RectF rect)
		: m_state(state)
		, m_rect(rect)
		, m_boardSize(state->getBoard()->getSize())
		, m_boardDrawer(m_rect, m_boardSize)
	{
		setRect(rect);
	}


	void GameVisualizer::setRect(RectF rect) {
		m_rect = rect;

		auto board = m_state->getBoard();
		Size boardSize = board->getSize();
		m_boardDrawer = Visualizer_01::BoardDrawer(m_rect, boardSize);
	}

	void GameVisualizer::draw() {
		using namespace Visualizer_01;


		auto rect = m_rect;
		rect.draw(Palette::Black);

		auto board = m_state->getBoard();
		Size boardSize = board->getSize();

		BoardDrawer boardDrawer(rect, boardSize);


		// Biome を描画
		for (int32 r = 0; r < board->getHeight(); r++) {
			for (int32 c = 0; c < board->getWidth(); c++) {
				auto pos = BoardPos(r, c);
				auto biome = (*board)[{ r, c }].biome;
				boardDrawer.drawBiome(biome, pos);
			}
		}

		// Area （陣地）を描画
		// Wall （城壁）を描画
		for (int32 r = 0; r < board->getHeight(); r++) {
			for (int32 c = 0; c < board->getWidth(); c++) {
				auto pos = BoardPos(r, c);

				// Area
				for (auto player : { PlayerColor::Red, PlayerColor::Blue }) {
					if (m_state->isClosedAreaOf(player, pos)) {
						boardDrawer.drawArea(player, true, pos);
					}
					else if (m_state->isAreaOf(player, pos)) {
						boardDrawer.drawArea(player, false, pos);
					}
				}

				// Wall
				auto wall = (*board)[pos].wall;
				if (wall.has_value()) {
					boardDrawer.drawWall(wall->color, pos);
				}
			}
		}

		// Agent （職人）を描画
		for (auto& agent : m_state->getAllAgents()) {
			auto pos = agent.pos;
			auto col = agent.marker.color;

			boardDrawer.drawAgent(col, pos);
		}

	}

	RectF GameVisualizer::getMassRect(BoardPos pos) {
		return m_boardDrawer.getMassRect(pos);
	}

	Optional<BoardPos> GameVisualizer::checkPos(Vec2 pos) {
		for (int r = 0; r < m_boardSize.y; r++) {
			for (int c = 0; c < m_boardSize.x; c++) {
				// マス (r,c) がその座標を含んでいたら (r,c) を返す
				if (getMassRect(BoardPos(r, c)).contains(pos)) {
					return BoardPos(r, c);
				}
			}
		}
		return none;
	}




	auto GameControllerVisualizer::beginTurn(PlayerColor player) {
		m_playerToMove = player;
		auto agents = m_game->getAgents(player);
		m_instructionBuffer.assign(agents.size(), AgentMove::GetNull());
		m_instructionBuffer = agents.map([&](Agent a) { return AgentMove::GetStay(a); });

		// m_selectedAgentId = -1;
		m_selectedAgentId = 0;
		m_agentCursor = none;
		m_directedArrowIcon = none;
	}

	// recalc based on m_globalRect
	void GameControllerVisualizer::recalcDrawArea() {
		m_gameVisRect = m_globalRect;

		Array<RectF> buttonRect(ButtonCount);

		if (m_globalRect.h > m_globalRect.w) {
			double controllerHeight = Min(70.0, m_globalRect.h * 0.25);
			m_gameVisRect.h -= controllerHeight;

			RectF controllerRect = m_globalRect;
			controllerRect.y += m_gameVisRect.h;
			controllerRect.h -= m_gameVisRect.h;

			double buttonHeight = controllerHeight * 0.9;
			double spc = controllerHeight * 0.05;

			buttonRect[0] = RectF(controllerRect.tl() + Vec2(spc, spc), Vec2(buttonHeight, buttonHeight));
			for (int i = 1; i < ButtonCount; i++) {
				buttonRect[i] = buttonRect[i - 1];
				buttonRect[i].x += buttonRect[i - 1].w + spc;
			}

			RectF controllerRealRect = RectF(buttonRect.front().tl(), buttonRect.back().br() - buttonRect.front().tl());
			for (auto& a : buttonRect) {
				Vec2 offset = (controllerRect.size - controllerRealRect.size) * 0.5;
				a.pos += offset;
			}
		}
		else {
			double controllerWidth = Min(70.0, m_globalRect.w * 0.25);
			m_gameVisRect.w -= controllerWidth;

			RectF controllerRect = m_globalRect;
			controllerRect.x += m_gameVisRect.w;
			controllerRect.w -= m_gameVisRect.w;

			double buttonHeight = controllerWidth * 0.9;
			double spc = controllerWidth * 0.05;

			buttonRect[0] = RectF(controllerRect.tl() + Vec2(spc, spc), Vec2(buttonHeight, buttonHeight));
			for (int i = 1; i < ButtonCount; i++) {
				buttonRect[i] = buttonRect[i - 1];
				buttonRect[i].y += buttonRect[i - 1].h + spc;
			}

			RectF controllerRealRect = RectF(buttonRect.front().tl(), buttonRect.back().br() - buttonRect.front().tl());
			for (auto& a : buttonRect) {
				Vec2 offset = (controllerRect.size - controllerRealRect.size) * 0.5;
				a.pos += offset;
			}
		}

		m_button0.setRect(buttonRect[0]);
		m_button1.setRect(buttonRect[1]);
		m_button2.setRect(buttonRect[2]);
		m_button3.setRect(buttonRect[3]);
		m_button4.setRect(buttonRect[4]);

	}

	GameControllerVisualizer::GameControllerVisualizer(BoxPtr<GameState> game, RectF rect)
		: m_game(game)
		, m_gameVis()
		, m_globalRect(rect)
		, m_selectedAgentId(-1)
	{

		m_button0 = FlickButton([](RectF rect) {
			rect.drawFrame(1.0, Palette::Red);
			GetArrowIcon(rect).draw();
		});
		m_button0.setEnabledDirection(0b0'11111111);
		m_button0.setLightingColor(Palette::Yellow);

		m_button1 = FlickButton([](RectF rect) {
			rect.drawFrame(1.0, Palette::Red);
			GetPlusIcon(rect).draw();
		});
		m_button1.setEnabledDirection(0b0'01010101);
		m_button1.setLightingColor(Palette::Yellow);

		m_button2 = FlickButton([](RectF rect) {
			rect.drawFrame(1.0, Palette::Red);
			GetMinusIcon(rect).draw();
		});
		m_button2.setEnabledDirection(0b0'01010101);
		m_button2.setLightingColor(Palette::Yellow);

		m_button3 = FlashingButton([](RectF rect) {
			rect.drawFrame(1.0, Palette::Red);
			GetCrossIcon(rect).draw();
		});
		m_button3.setLightingColor(Palette::Yellow);

		m_button4 = FlashingButton([](RectF rect) {
			rect.drawFrame(1.0, Palette::Red);
			GetCheckIcon(rect).draw();
		});
		m_button4.setLightingColor(Palette::Yellow);

		recalcDrawArea();

		m_gameVis = std::make_shared<GameVisualizer>(m_game, m_gameVisRect);

		if (!m_game->isOver()) {
			beginTurn(m_game->whosTurn());
		}
	}

	Polygon GameControllerVisualizer::GetArrowIcon(RectF rect, double angle) {
		static Polygon base = Shape2D::Arrow(Line(Vec2(0.15, 0.5), Vec2(0.85, 0.5)), 0.3, Vec2(0.3, 0.3));
		return base.rotatedAt(Vec2(0.5, 0.5), -angle).scaled(rect.size).movedBy(rect.tl());
	}

	Polygon GameControllerVisualizer::GetPlusIcon(RectF rect) {
		static Polygon base = Shape2D::Cross(0.35, 0.18, Vec2(0.5, 0.5), 45.0_deg);
		return base.scaled(rect.size).movedBy(rect.tl());
	}

	Polygon GameControllerVisualizer::GetMinusIcon(RectF rect) {
		static Polygon base = RectF(Vec2(0.15, 0.41), Vec2(0.7, 0.18)).asPolygon();
		return base.scaled(rect.size).movedBy(rect.tl());
	}

	Polygon GameControllerVisualizer::GetCrossIcon(RectF rect) {
		static Polygon base = Shape2D::Cross(0.35, 0.18, Vec2(0.5, 0.5), 0.0_deg);
		return base.scaled(rect.size).movedBy(rect.tl());
	}

	Polygon GameControllerVisualizer::GetCheckIcon(RectF rect) {
		static Polygon base = LineString({ Vec2(-0.30, -0.03), Vec2(-0.10, 0.22), Vec2(0.30, -0.28) }).calculateBuffer(0.08);
		double x = Min(rect.h, rect.w);
		return base.scaled(x).movedBy(rect.center());
	}

	void GameControllerVisualizer::setRect(RectF rect) {
		m_globalRect = rect;
		recalcDrawArea();
		if(m_gameVis) m_gameVis->setRect(m_gameVisRect);
	}

	void GameControllerVisualizer::update() {

		int wayR = 0, wayC = 0;
		if (KeyUp.pressed()) wayR -= 1;
		if (KeyDown.pressed()) wayR += 1;
		if (KeyLeft.pressed()) wayC -= 1;
		if (KeyRight.pressed()) wayC += 1;
		if (wayR == 0 && wayC == 0) m_selectedDirection = none;
		if (wayR == 0 && wayC == 1) m_selectedDirection = MoveDirection::Right;
		if (wayR == -1 && wayC == 1) m_selectedDirection = MoveDirection::RightUp;
		if (wayR == -1 && wayC == 0) m_selectedDirection = MoveDirection::Up;
		if (wayR == -1 && wayC == -1) m_selectedDirection = MoveDirection::LeftUp;
		if (wayR == 0 && wayC == -1) m_selectedDirection = MoveDirection::Left;
		if (wayR == 1 && wayC == -1) m_selectedDirection = MoveDirection::LeftDown;
		if (wayR == 1 && wayC == 0) m_selectedDirection = MoveDirection::Down;
		if (wayR == 1 && wayC == 1) m_selectedDirection = MoveDirection::RightDown;


		// エージェントの選択・カーソルの処理
		if (m_playerToMove.has_value()) {

			auto agents = m_game->getAgents(*m_playerToMove);

			Array<Input> agentKeys = { Key1, Key2, Key3, Key4, Key5, Key6 };
			for (size_t i = 0; i < agentKeys.size(); i++) {
				if (agentKeys[i].down() && i < agents.size()) {
					m_selectedAgentId = (int32)i;
				}
			}

			m_agentCursor = none;
			m_directedArrowIcon = none;
			m_directiedArrowIconColor = Palette::Yellow;

			if (0 <= m_selectedAgentId && m_selectedAgentId < (int32)agents.size()) {
				auto& agent = agents[m_selectedAgentId];

				m_agentCursor = m_gameVis->getMassRect(agent.pos);

				if (m_selectedDirection.has_value()) {
					auto direction = m_selectedDirection.value();
					auto newPos = agent.pos.movedAlong(direction);
					if (m_game->getBoard()->isOnBoard(newPos)) {
						m_directedArrowIcon = GetArrowIcon(m_gameVis->getMassRect(newPos), 45.0_deg * direction.value());
					}
				}
			}
		}


		m_button0.update();
		m_button1.update();
		m_button2.update();
		m_button3.update();
		m_button4.update();

		bool buttonProcessed = false;

		if (KeyQ.down() || m_button0.isTriggered()) {
			m_button0.setLightingColor(Palette::Red, 1.0);

			if (m_playerToMove.has_value()) {
				auto player = *m_playerToMove;
				auto agents = m_game->getAgents(player);

				int32 direction = -1;
				if (m_selectedDirection.has_value()) direction = m_selectedDirection->value();
				if (m_button0.isTriggered()) direction = m_button0.getDirection();

				if (0 <= m_selectedAgentId && m_selectedAgentId < (int32)agents.size()) {
					auto& agent = agents[m_selectedAgentId];

					if (0 <= direction
						&& direction <= 7
						&& m_game->getBoard()->isOnBoard(agent.pos.movedAlong(direction)))
					{
						m_instructionBuffer[m_selectedAgentId] = AgentMove::GetMove(agent, direction);
						m_button0.setLightingColor(SuccessColor);
						buttonProcessed = true;
					}
				}
			}
		}

		if (KeyW.down() || m_button1.isTriggered()) {
			m_button1.setLightingColor(Palette::Red, 1.0);

			if (m_playerToMove.has_value()) {
				auto player = *m_playerToMove;
				auto agents = m_game->getAgents(player);

				int32 direction = -1;
				if (m_selectedDirection.has_value()) direction = m_selectedDirection->value();
				if (m_button1.isTriggered()) direction = m_button1.getDirection();

				if (0 <= m_selectedAgentId && m_selectedAgentId < (int32)agents.size()) {
					auto& agent = agents[m_selectedAgentId];

					if (0 <= direction
						&& direction <= 7
						&& MoveDirection(direction).is4Direction()
						&& m_game->getBoard()->isOnBoard(agent.pos.movedAlong(direction)))
					{
						m_instructionBuffer[m_selectedAgentId] = AgentMove::GetConstruct(agent, direction);
						m_button1.setLightingColor(SuccessColor);
						buttonProcessed = true;
					}
				}
			}
		}

		if (KeyE.down() || m_button2.isTriggered()) {
			m_button2.setLightingColor(Palette::Red, 1.0);

			if (m_playerToMove.has_value()) {
				auto player = *m_playerToMove;
				auto agents = m_game->getAgents(player);

				int32 direction = -1;
				if (m_selectedDirection.has_value()) direction = m_selectedDirection->value();
				if (m_button2.isTriggered()) direction = m_button2.getDirection();

				if (0 <= m_selectedAgentId && m_selectedAgentId < (int32)agents.size()) {
					auto& agent = agents[m_selectedAgentId];

					if (0 <= direction
						&& direction <= 7
						&& MoveDirection(direction).is4Direction()
						&& m_game->getBoard()->isOnBoard(agent.pos.movedAlong(direction)))
					{
						m_instructionBuffer[m_selectedAgentId] = AgentMove::GetDestroy(agent, direction);
						m_button2.setLightingColor(SuccessColor);
						buttonProcessed = true;
					}
				}
			}
		}

		if (KeyR.down() || m_button3.clicked()) {
			m_button3.setLightingColor(Palette::Red, 1.0);

			if (m_playerToMove.has_value()) {
				auto player = *m_playerToMove;
				auto agents = m_game->getAgents(player);

				if (0 <= m_selectedAgentId && m_selectedAgentId < (int32)agents.size()) {
					auto& agent = agents[m_selectedAgentId];

					m_instructionBuffer[m_selectedAgentId] = AgentMove::GetStay(agent);
					m_button3.setLightingColor(SuccessColor);
					buttonProcessed = true;
				}
			}
		}

		if (KeyEnter.down() || m_button4.clicked()) {
			m_button4.setLightingColor(Palette::Red, 1.0);

			if (m_playerToMove.has_value()) {
				auto player = *m_playerToMove;
				auto agents = m_game->getAgents(player);

				auto instructionBuffer = TurnInstruction(m_game, player);
				for (size_t i = 0; i < agents.size(); i++) {
					instructionBuffer.insert(m_instructionBuffer[i]);
				}

				m_game->makeMove(player, instructionBuffer);
				auto nextPlayer = GameState::OpponentOf(*m_playerToMove);
				m_button4.setLightingColor(SuccessColor);

				if (!m_game->isOver()) {
					beginTurn(nextPlayer);
				}
				else {
					m_playerToMove = none;
					return;
				}
			}
		}


		if (m_playerToMove.has_value() && !buttonProcessed && KeyTab.down()) {
			auto player = *m_playerToMove;
			auto agents = m_game->getAgents(player);
			int32 agentsNum = (int32)agents.size();
			if (agentsNum > 0) {
				m_selectedAgentId++;
				m_selectedAgentId = Max(0, m_selectedAgentId) % agentsNum;
			}
		}


		// decoration
		if (m_playerToMove.has_value()) {
			auto player = *m_playerToMove;
			auto agents = m_game->getAgents(player);

			if (0 <= m_selectedAgentId && m_selectedAgentId < (int32)agents.size()) {
				auto& agent = agents[m_selectedAgentId];

				if (m_selectedDirection.has_value()) {
					auto direction = m_selectedDirection.value();
					if (m_game->getBoard()->isOnBoard(agent.pos.movedAlong(direction))) {

						if (!buttonProcessed && KeyW.pressed() && !direction.is4Direction()) {
							m_directiedArrowIconColor = Palette::Orangered;
							buttonProcessed = true;
						}
						if (!buttonProcessed && KeyE.pressed() && !direction.is4Direction()) {
							m_directiedArrowIconColor = Palette::Orangered;
							buttonProcessed = true;
						}

					}
				}
			}

		}

	}

	void GameControllerVisualizer::draw() {

		auto GetInnerRectangleFrame = [](RectF rect, double width) -> Polygon {
			return Polygon(
				{ Vec2(0.0, 0.0), Vec2(1.0, 0.0), Vec2(1.0, 1.0), Vec2(0.0, 1.0) },
				{ { Vec2(width, width), Vec2(width, 1.0 - width), Vec2(1.0 - width, 1.0 - width), Vec2(1.0 - width, width) } }
			).scaled(rect.size).movedBy(rect.tl());
		};


		// render start

		m_gameVis->draw();

		if (!m_game->isOver()) {

			for (auto& instruction : m_instructionBuffer) {
				if (instruction.isStay()) continue;
				if (instruction.isMove()) {
					auto inst = instruction.asMove();
					auto prevPosRect = m_gameVis->getMassRect(inst.agent.pos);
					auto newPosRect = m_gameVis->getMassRect(inst.agent.pos.movedAlong(inst.dir));
					auto icon1 = GetArrowIcon(newPosRect, 45.0_deg * inst.dir.value());
					auto icon2 = GetInnerRectangleFrame(prevPosRect, 0.1);
					icon1.draw(Palette::Lime);
					icon2.draw(Palette::Lime);
				}
				if (instruction.isConstruct()) {
					auto inst = instruction.asConstruct();
					auto prevPosRect = m_gameVis->getMassRect(inst.agent.pos);
					auto newPosRect = m_gameVis->getMassRect(inst.agent.pos.movedAlong(inst.dir));
					auto icon1 = GetPlusIcon(newPosRect);
					auto icon2 = GetInnerRectangleFrame(prevPosRect, 0.1);
					icon1.draw(Palette::Lime);
					icon2.draw(Palette::Lime);
				}
				if (instruction.isDestroy()) {
					auto inst = instruction.asDestroy();
					auto prevPosRect = m_gameVis->getMassRect(inst.agent.pos);
					auto newPosRect = m_gameVis->getMassRect(inst.agent.pos.movedAlong(inst.dir));
					auto icon1 = GetMinusIcon(newPosRect);
					auto icon2 = GetInnerRectangleFrame(prevPosRect, 0.1);
					icon1.draw(Palette::Lime);
					icon2.draw(Palette::Lime);
				}
			}

			// カーソルの描画
			if (m_agentCursor.has_value()) m_agentCursor->drawFrame(3.0, Palette::Yellow);
			if (m_directedArrowIcon.has_value()) m_directedArrowIcon->draw(m_directiedArrowIconColor);

		}

		m_button0.draw();
		m_button1.draw();
		m_button2.draw();
		m_button3.draw();
		m_button4.draw();

	}


}
