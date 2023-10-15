#pragma once
#include "../game_util.hpp"
#include "../game_state.hpp"
#include "../gui/integer_textbox.hpp"
#include "../gui/tab_menu.hpp"

namespace Procon34 {


	namespace MapEditor_01 {


		class MapEditor {
		public:

			MapEditor();

			// 更新と描画
			void update(RectF rect);

			Optional<GameInitialState> getData();

		private:

			void updateSettingMode(RectF rect);

			void updateMapMode(RectF rect);

			void prepareMapMode();


			Font m_font;

			size_t m_editorDisplayFlip = 0;

			Gui::IntegerTextBox m_intBoxWidth;
			Gui::IntegerTextBox m_intBoxHeight;
			Gui::IntegerTextBox m_intBoxWallValue;
			Gui::IntegerTextBox m_intBoxAreaValue;
			Gui::IntegerTextBox m_intBoxCastleValue;
			Gui::IntegerTextBox m_intBoxTurnCount;
			Gui::IntegerTextBox m_intBoxTimeLimit;
			size_t m_firstToMoveRadioButtonsSelected = 0;

			EachPlayer<Array<BoardPos>> m_agentPosition;

			Grid<MassBiome> m_biomeGrid;
			Grid<Optional<PlayerColor>> m_agentHolder;

			size_t m_mapPainter;

			Gui::TabMenu m_menuBar;

			Optional<String> isSettingOkOrErrmsg() const;

		};

	} // namespace MapEditor_01

} // namespace Procon34
