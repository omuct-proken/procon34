#include "../map_editor_01.hpp"
#include "../../module_visualize/ver_01.hpp"

namespace Procon34 {


	namespace MapEditor_01 {


		MapEditor::MapEditor()
			: m_font(25)
			, m_editorDisplayFlip(0)
			, m_intBoxWidth(U"フィールドの幅", 1, 50, 15)
			, m_intBoxHeight(U"フィールドの高さ", 1, 50, 15)
			, m_intBoxWallValue(U"城壁係数", 1, 100, 10)
			, m_intBoxAreaValue(U"陣地係数", 1, 100, 30)
			, m_intBoxCastleValue(U"城係数", 1, 100, 50)
			, m_intBoxTurnCount(U"ターン数", 1, 10000, 100)
			, m_intBoxTimeLimit(U"時間制限 (ms) ", 100, 100000, 5000)
			, m_mapPainter(0)
			, m_menuBar({ U"試合設定", U"マップ" })
		{
		}

		void MapEditor::updateSettingMode(RectF rect) {
			const Vec2 VerticalStep = Vec2(0.0, 45.0);
			const double TextHorizontalLength = 200.0;

			Vec2 startPos = rect.tl() + Vec2(20.0 + TextHorizontalLength, 20.0);

			Vec2 iterPos = startPos;
			Vec2 textboxSize = Vec2(80.0, VerticalStep.y);

			m_intBoxWidth.update(RectF(iterPos, textboxSize));
			m_font(U"ﾌｨｰﾙﾄﾞの幅：").draw(Arg::topRight(iterPos));

			iterPos += VerticalStep;
			m_intBoxHeight.update(RectF(iterPos, textboxSize));
			m_font(U"ﾌｨｰﾙﾄﾞの高さ：").draw(Arg::topRight(iterPos));

			iterPos += VerticalStep;
			m_intBoxWallValue.update(RectF(iterPos, textboxSize));
			m_font(U"城壁係数：").draw(Arg::topRight(iterPos));

			iterPos += VerticalStep;
			m_intBoxAreaValue.update(RectF(iterPos, textboxSize));
			m_font(U"陣地係数：").draw(Arg::topRight(iterPos));

			iterPos += VerticalStep;
			m_intBoxCastleValue.update(RectF(iterPos, textboxSize));
			m_font(U"城係数：").draw(Arg::topRight(iterPos));

			iterPos += VerticalStep;
			m_intBoxTurnCount.update(RectF(iterPos, textboxSize));
			m_font(U"ターン数：").draw(Arg::topRight(iterPos));

			iterPos += VerticalStep;
			m_intBoxTimeLimit.update(RectF(iterPos, textboxSize));
			m_font(U"時間制限 (ms) ：").draw(Arg::topRight(iterPos));

			iterPos += VerticalStep;
			SimpleGUI::HorizontalRadioButtons(m_firstToMoveRadioButtonsSelected, { U"赤", U"青" }, iterPos);
			m_font(U"先攻：").draw(Arg::topRight(iterPos));

		}

		void MapEditor::updateMapMode(RectF rect) {

			double ySeparate = rect.y + rect.h * 0.75;
			RectF boardRect = RectF(rect.x, rect.y, rect.w, ySeparate - rect.y);
			RectF paletteRect = RectF(rect.x, ySeparate, rect.w, rect.y + rect.h - ySeparate);

			int32 boardHeight = (int32)m_intBoxHeight.value();
			int32 boardWidth = (int32)m_intBoxWidth.value();
			Size boardSize = BoardPos(boardHeight, boardWidth).asPoint();
			auto boardDrawer = Visualizer_01::BoardDrawer(boardRect, boardSize);

			if (Key1.down()) {
				m_mapPainter = 0;
			}
			if (Key2.down()) {
				m_mapPainter = 1;
			}
			if (Key3.down()) {
				m_mapPainter = 2;
			}
			if (Key4.down()) {
				m_mapPainter = 3;
			}
			if (Key5.down()) {
				m_mapPainter = 4;
			}

			if (boardRect.mouseOver()) {
				Optional<BoardPos> massPosOpt = none;
				for (int r = 0; r < boardHeight; r++) {
					for (int c = 0; c < boardWidth; c++) {
						auto boardPos = BoardPos(r, c);
						auto massRect = boardDrawer.getMassRect(boardPos);
						if (massRect.contains(Cursor::PosF())) {
							massPosOpt = boardPos;
						}
					}
				}

				if (massPosOpt.has_value()) {
					BoardPos massPos = massPosOpt.value();

					if (MouseL.pressed()) {
						switch (m_mapPainter) {

						case 0:
							m_biomeGrid[massPos.asPoint()] = MassBiome::Normal;
							break;

						case 1:
							m_biomeGrid[massPos.asPoint()] = MassBiome::Pond;
							break;

						case 2:
							m_biomeGrid[massPos.asPoint()] = MassBiome::Castle;
							break;

						}
					}

					if (MouseL.down()) {
						switch (m_mapPainter) {

						case 3:
						{
							auto& targetRef = m_agentHolder[massPos.asPoint()];
							if (targetRef.value_or(PlayerColor::Red) == PlayerColor::Blue) {
								targetRef = none;
							}
							else {
								targetRef = PlayerColor::Blue;
							}
						} break;

						case 4:
						{
							auto& targetRef = m_agentHolder[massPos.asPoint()];
							if (targetRef.value_or(PlayerColor::Blue) == PlayerColor::Red) {
								targetRef = none;
							}
							else {
								targetRef = PlayerColor::Red;
							}
						} break;

						}
					}
				}
			}


			// 描画 開始

			boardDrawer.drawFullBiome(m_biomeGrid);

			for (int r = 0; r < boardHeight; r++) {
				for (int c = 0; c < boardWidth; c++) {
					auto pos = BoardPos(r, c);
					if (m_agentHolder[pos.asPoint()].has_value()) {
						boardDrawer.drawAgent(m_agentHolder[pos.asPoint()].value(), pos);
					}
				}
			}

		}

		void MapEditor::prepareMapMode() {
			int32 boardHeight = (int32)m_intBoxHeight.value();
			int32 boardWidth = (int32)m_intBoxWidth.value();
			Size boardSize = BoardPos(boardHeight, boardWidth).asPoint();
			m_biomeGrid.resize(boardSize, MassBiome::Normal);
			m_agentHolder.resize(boardSize, none);
		}

		// 更新と描画
		void MapEditor::update(RectF rect) {

			double menuHeight = m_menuBar.getRegionFromGlobalRect(rect).h;
			m_menuBar.update(rect);

			RectF inRect = RectF(rect.x, rect.y + menuHeight, rect.w, rect.h - menuHeight);

			switch (m_editorDisplayFlip) {

			case 0:
				updateSettingMode(inRect);
				break;

			case 1:
				updateMapMode(inRect);
				break;
			}

			size_t nextEditorDisplayFlip = m_menuBar.getValue();

			if (m_editorDisplayFlip != nextEditorDisplayFlip) {
				m_editorDisplayFlip = nextEditorDisplayFlip;

				auto possiblyErrmsg = isSettingOkOrErrmsg();
				if (possiblyErrmsg.has_value()) {
					System::MessageBoxOK(U"ダメ", U"試合設定に無効なところがあります。：\n" + possiblyErrmsg.value());
					m_editorDisplayFlip = 0;
				}

				if (m_editorDisplayFlip == 1) {
					prepareMapMode();
				}
			}

			m_menuBar.draw(rect);

		}

		Optional<GameInitialState> MapEditor::getData() {
			auto res = GameInitialState();
			res.firstToMove = m_firstToMoveRadioButtonsSelected ? PlayerColor::Blue : PlayerColor::Red;

			if (!m_intBoxWidth.isOk()) return none;
			res.boardWidth = (int32)m_intBoxWidth.value();

			if (!m_intBoxHeight.isOk()) return none;
			res.boardHeight = (int32)m_intBoxHeight.value();

			if (!m_intBoxWallValue.isOk()) return none;
			res.wallCoefficient = (int32)m_intBoxWallValue.value();

			if (!m_intBoxAreaValue.isOk()) return none;
			res.teritorryCoefficient = (int32)m_intBoxAreaValue.value();

			if (!m_intBoxCastleValue.isOk()) return none;
			res.castleCoefficient = (int32)m_intBoxCastleValue.value();

			if (!m_intBoxTurnCount.isOk()) return none;
			res.turnCount = (int32)m_intBoxTurnCount.value();

			if (!m_intBoxTimeLimit.isOk()) return none;
			res.turnTimeLimitInMiliseconds = (int32)m_intBoxTimeLimit.value();

			res.biomeGrid = m_biomeGrid;

			m_agentPosition[PlayerColor::Red].clear();
			m_agentPosition[PlayerColor::Blue].clear();
			for (int r = 0; r < res.boardHeight; r++) {
				for (int c = 0; c < res.boardWidth; c++) {
					if (m_agentHolder[Size(c, r)].has_value()) {
						m_agentPosition[m_agentHolder[Size(c, r)].value()].push_back(BoardPos(r, c));
					}
				}
			}
			res.agentPos = m_agentPosition;

			return res;
		}

		Optional<String> MapEditor::isSettingOkOrErrmsg() const {
			Array<String> res;
			if (!m_intBoxWidth.isOk()) res.push_back(m_intBoxWidth.getErrorMessage());
			if (!m_intBoxHeight.isOk()) res.push_back(m_intBoxHeight.getErrorMessage());
			if (!m_intBoxWallValue.isOk()) res.push_back(m_intBoxWallValue.getErrorMessage());
			if (!m_intBoxAreaValue.isOk()) res.push_back(m_intBoxAreaValue.getErrorMessage());
			if (!m_intBoxCastleValue.isOk()) res.push_back(m_intBoxCastleValue.getErrorMessage());
			if (!m_intBoxTurnCount.isOk()) res.push_back(m_intBoxTurnCount.getErrorMessage());
			if (!m_intBoxTimeLimit.isOk()) res.push_back(m_intBoxTimeLimit.getErrorMessage());

			if (res.empty()) return none;
			String resstr = res[0];
			for (size_t i = 1; i < res.size(); i++) {
				resstr += U"\n";
				resstr += res[i];
			}
			return resstr;
		}


	} // namespace MapEditor_01

} // namespace Procon34
