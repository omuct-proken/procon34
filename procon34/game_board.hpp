#pragma once
#include "stdafx.h"
#include "game_util.hpp"
#include "game_agent.hpp"



namespace Procon34 {

	// 壁 1 つが持つ情報
	struct WallData {
		PlayerColor color;
	};


	// マス 1 つが持つ情報
	struct Mass {
		Optional<AgentMarker> agent;
		Optional<WallData> wall;
		MassBiome biome = MassBiome::Normal;

		// 壁があるか？
		bool hasWall() const noexcept { return wall.has_value(); }

		// 職人がいるか？
		bool hasAgent() const noexcept { return agent.has_value(); }

		// 色 color の壁があるか？
		bool hasWallOf(PlayerColor color) const noexcept { return wall.has_value() && wall->color == color; }
	};


	// 盤面がもつすべてのマスを管理する
	class GameBoard {
	private:
		int32 m_width;
		int32 m_height;
		Array<Mass> m_mass;

		int32 posToArrayIdx(BoardPos pos) const noexcept { return pos.r * getWidth() + pos.c; }
	public:

		GameBoard(int32 width, int32 height)
			: m_width(width)
			, m_height(height)
		{
			if (width < 1) throw Error(U"GameBoard::GameBoard out of range ( width < 1 )");
			if (500 < width) throw Error(U"GameBoard::GameBoard out of range ( 500 < width )");
			if (height < 1) throw Error(U"GameBoard::GameBoard out of range ( height < 1 )");
			if (500 < height) throw Error(U"GameBoard::GameBoard out of range ( 500 < height )");

			m_mass.resize(width * height);
		}

		int32 getWidth() const noexcept { return m_width; }

		int32 getHeight() const noexcept { return m_height; }

		// (x,y) = (幅, 高さ)
		Size getSize() const noexcept { return Size(m_width, m_height); }

		// 盤面に座標 pos が存在するか？
		bool isOnBoard(BoardPos pos) const noexcept {
			if (pos.r < 0 || getHeight() <= pos.r || pos.c < 0 || getWidth() <= pos.c) return false;
			return true;
		}

		Mass getMassOr(BoardPos pos, Mass ex) const noexcept {
			return isOnBoard(pos) ? m_mass[posToArrayIdx(pos)] : ex;
		}

		// 座標 pos にあるマスを取得
		Mass getMass(BoardPos pos) const {
			if (!isOnBoard(pos)) throw Error(U"GameBoard::getMass isOnBoard(r, c) failed");
			return m_mass[posToArrayIdx(pos)];
		}

		// 座標を指定してマスにアクセス
		Mass& operator[](BoardPos pos) {
			if (!isOnBoard(pos)) {
				throw Error(U"GameBoard::operator[] isOnBoard(r, c) failed");
			}
			return m_mass[posToArrayIdx(pos)];
		}

		// 座標を指定してマスにアクセス
		const Mass& operator[](BoardPos pos) const {
			if (!isOnBoard(pos)) throw Error(U"GameBoard::operator[] isOnBoard(r, c) failed");
			return m_mass[posToArrayIdx(pos)];
		}

	};

}
