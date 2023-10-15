#pragma once

#include "../ver_01.hpp"


namespace Procon34 {

	namespace Visualizer_01 {



		MassDrawer::MassDrawer() {
		}

		Polygon MassDrawer::GetInnerRectangleFrame(double width, RectF rect) {
			return Polygon(
				{ Vec2(0.0, 0.0), Vec2(1.0, 0.0), Vec2(1.0, 1.0), Vec2(0.0, 1.0) },
				{ { Vec2(width, width), Vec2(width, 1.0 - width), Vec2(1.0 - width, 1.0 - width), Vec2(1.0 - width, width) } }
			).scaled(rect.size).movedBy(rect.tl());
		}

		Polygon MassDrawer::GetCheckPattern(double pos, double width, RectF rect) {
			width /= 2.0;
			return Polygon(
				{
					Vec2(pos - width, pos - width),
					Vec2(pos - width,         0.0),
					Vec2(pos + width,         0.0),
					Vec2(pos + width, pos - width),
					Vec2(1.0, pos - width),
					Vec2(1.0, pos + width),
					Vec2(pos + width, pos + width),
					Vec2(pos + width,         1.0),
					Vec2(pos - width,         1.0),
					Vec2(pos - width, pos + width),
					Vec2(0.0, pos + width),
					Vec2(0.0, pos - width)
				}
			).scaled(rect.size).movedBy(rect.tl());
		}

		void MassDrawer::drawNormalBiome(RectF rect) {
			rect.scaledAt(rect.center(), 0.9).draw(Palette::Lime.withAlpha(100));
		}

		void MassDrawer::drawPondBiome(RectF rect) {
			rect.scaledAt(rect.center(), 0.9).draw(Palette::Skyblue.withAlpha(50));
		}

		void MassDrawer::drawCastleBiome(RectF rect) {
			rect.scaledAt(rect.center(), 0.9).draw(Palette::Lime.withAlpha(100));
			Shape2D::Star(Min(rect.w, rect.h) * 0.3, rect.center()).draw();
		}

		void MassDrawer::drawBiome(MassBiome biome, RectF rect) {
			if (biome == MassBiome::Normal) drawNormalBiome(rect);
			if (biome == MassBiome::Pond) drawPondBiome(rect);
			if (biome == MassBiome::Castle) drawCastleBiome(rect);
		}

		void MassDrawer::drawArea(PlayerColor color, bool closed, RectF rect) {
			if (color == PlayerColor::Red) {
				CheckPatternRed.scaled(rect.size).movedBy(rect.tl()).draw(Palette::Red.withAlpha(closed ? 100 : 50));
			}
			else {
				CheckPatternBlue.scaled(rect.size).movedBy(rect.tl()).draw(Palette::Blue.withAlpha(closed ? 100 : 50));
			}
		}

		void MassDrawer::drawWall(PlayerColor color, RectF rect) {
			ColorF drawColor = (color == PlayerColor::Red ? Palette::Red : Palette::Blue);
			InnerRectangleFrame1.scaled(rect.size).movedBy(rect.tl()).draw(drawColor);
		}

		void MassDrawer::drawAgent(PlayerColor color, RectF rect) {
			double sz = Min(rect.w, rect.h);

			auto shape = Shape2D::Hexagon(sz * 0.3, rect.center());
			shape.drawFrame(5.0, Palette::White.withAlpha(150));
			if (color == PlayerColor::Red) shape.drawFrame(3.0, Palette::Red);
			if (color == PlayerColor::Blue) shape.drawFrame(3.0, Palette::Blue);
		}




		BoardDrawer::BoardDrawer(RectF outerRect, Size boardSize) {

			double dispMassLength = Min(outerRect.h / boardSize.y, outerRect.w / boardSize.x);
			Vec2 dispMassSize = Vec2(dispMassLength, dispMassLength);

			Grid<RectF> massRects(boardSize);
			Vec2 dispLT = outerRect.center() - Vec2(boardSize) * (dispMassLength * 0.5);
			for (int32 r = 0; r < boardSize.y; r++) {
				for (int32 c = 0; c < boardSize.x; c++) {
					auto pos = BoardPos(r, c);
					massRects[pos.asPoint()] = RectF(dispLT + Vec2(dispMassLength * c, dispMassLength * r), dispMassSize);
				}
			}

			m_massRect = std::move(massRects);
		}

		RectF BoardDrawer::getMassRect(BoardPos pos) {
			return m_massRect[pos.asPoint()];
		}

		void BoardDrawer::drawNormalBiome(BoardPos pos) {
			m_massDrawer.drawNormalBiome(getMassRect(pos));
		}

		void BoardDrawer::drawPondBiome(BoardPos pos) {
			m_massDrawer.drawPondBiome(getMassRect(pos));
		}

		void BoardDrawer::drawCastleBiome(BoardPos pos) {
			m_massDrawer.drawCastleBiome(getMassRect(pos));
		}

		void BoardDrawer::drawBiome(MassBiome biome, BoardPos pos) {
			m_massDrawer.drawBiome(biome, getMassRect(pos));
		}

		void BoardDrawer::drawArea(PlayerColor color, bool closed, BoardPos pos) {
			m_massDrawer.drawArea(color, closed, getMassRect(pos));
		}

		void BoardDrawer::drawWall(PlayerColor color, BoardPos pos) {
			m_massDrawer.drawWall(color, getMassRect(pos));
		}

		void BoardDrawer::drawAgent(PlayerColor color, BoardPos pos) {
			m_massDrawer.drawAgent(color, getMassRect(pos));
		}

		void BoardDrawer::drawFullBiome(const Grid<MassBiome>& grid) {
			for (int r = 0; r < grid.height(); r++) {
				for (int c = 0; c < grid.width(); c++) {
					auto pos = BoardPos(r, c);
					drawBiome(grid[pos.asPoint()], pos);
				}
			}
		}


	} // namespace Visualizer


}

