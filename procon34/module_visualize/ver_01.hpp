#pragma once

#include "../game_state.hpp"


namespace Procon34 {

	namespace Visualizer_01 {


		class MassDrawer {
		public:

			MassDrawer();

			void drawNormalBiome(RectF rect);
			void drawPondBiome(RectF rect);
			void drawCastleBiome(RectF rect);

			void drawBiome(MassBiome biome, RectF rect);
			void drawArea(PlayerColor color, bool closed, RectF rect);
			void drawWall(PlayerColor color, RectF rect);
			void drawAgent(PlayerColor color, RectF rect);

		private:

			static constexpr RectF IdentityRect = RectF(0.0, 0.0, 1.0, 1.0);

			Polygon GetInnerRectangleFrame(double width, RectF rect = IdentityRect);
			Polygon GetCheckPattern(double pos, double width, RectF rect = IdentityRect);

			Polygon CheckPatternRed = GetCheckPattern(0.4, 0.20);
			Polygon CheckPatternBlue = GetCheckPattern(0.6, 0.20);
			Polygon InnerRectangleFrame1 = GetInnerRectangleFrame(0.15);
		};

		class BoardDrawer {
		public:

			BoardDrawer(RectF outerRect, Size boardSize);

			RectF getMassRect(BoardPos pos);

			void drawNormalBiome(BoardPos pos);
			void drawPondBiome(BoardPos pos);
			void drawCastleBiome(BoardPos pos);

			void drawBiome(MassBiome biome, BoardPos pos);
			void drawArea(PlayerColor color, bool closed, BoardPos pos);
			void drawWall(PlayerColor color, BoardPos pos);
			void drawAgent(PlayerColor color, BoardPos pos);

			void drawFullBiome(const Grid<MassBiome>& grid);

		private:

			Grid<RectF> m_massRect;

			MassDrawer m_massDrawer;

			static constexpr RectF IdentityRect = RectF(0.0, 0.0, 1.0, 1.0);

		};

	} // namespace Visualizer


}

