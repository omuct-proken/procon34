#pragma once

#include "stdafx.h"
#include "dsu_fast.hpp"
#include "game_util.hpp"
#include "game_state.hpp"
#include "game_instructions.hpp"
#include "game_visualizer_buttons.hpp"
#include "module_visualize/ver_01.hpp"


namespace Procon34 {


	class GameVisualizer {
	public:

		// state : 描画する試合
		// rect : 描画する範囲
		GameVisualizer(BoxPtr<GameState> state, RectF rect);

		void setRect(RectF rect);

		// 試合の状態を描画する
		void draw();

		// マス pos が描画される領域を取得する
		RectF getMassRect(BoardPos pos);

		// 画面上の点 pos を含むマスがあれば、それを返す
		Optional<BoardPos> checkPos(Vec2 pos);

	private:

		BoxPtr<GameState> m_state;
		RectF m_rect;
		Size m_boardSize;

		Visualizer_01::BoardDrawer m_boardDrawer;
	};


	class GameControllerVisualizer {
	public:

		GameControllerVisualizer(BoxPtr<GameState> game, RectF rect);

		// 便利な記号を定義しています
		// マスを描画する範囲を rect として、図形を Polygon で返します

		static Polygon GetArrowIcon(RectF rect, double angle = 0.0); // angle は ***.0_deg で設定するのがおすすめ
		static Polygon GetPlusIcon(RectF rect);
		static Polygon GetMinusIcon(RectF rect);
		static Polygon GetCrossIcon(RectF rect);
		static Polygon GetCheckIcon(RectF rect);

		// 盤面と操作盤を描画してもよい範囲を設定する
		void setRect(RectF rect);

		// 操作盤の操作を受け付けたりする
		void update();

		// 描画する
		void draw();

	private:
		static inline constexpr size_t ButtonCount = 5;
		static inline constexpr Color SuccessColor = Palette::Cyan;

		BoxPtr<GameState> m_game;
		BoxPtr<GameVisualizer> m_gameVis;

		RectF m_globalRect;
		RectF m_gameVisRect;

		FlickButton m_button0;
		FlickButton m_button1;
		FlickButton m_button2;
		FlashingButton m_button3;
		FlashingButton m_button4;

		Optional<PlayerColor> m_playerToMove;
		int32 m_selectedAgentId;
		Optional<MoveDirection> m_selectedDirection;
		Array<AgentMove> m_instructionBuffer;
		Optional<RectF> m_agentCursor;
		Optional<Polygon> m_directedArrowIcon;
		Color m_directiedArrowIconColor;

		auto beginTurn(PlayerColor player);

		// recalc based on m_globalRect
		void recalcDrawArea();
	};


	// opt に入っている文字列を連続して描画する
	// 描画位置はつなげて描画したときと同じで、色だけ個別に設定する
	// basePos は左上位置
	inline void RenderTextColored(
		Font font,
		const Array<std::pair<String, Color>>& opt,
		Vec2 basePos
	) {

		Vec2 penPos{ basePos };

		for (const auto& [str, col] : opt) {

			// https://zenn.dev/reputeless/books/siv3d-documentation/viewer/tutorial-font#14.19-%E6%96%87%E5%AD%97%E5%8D%98%E4%BD%8D%E3%81%A7%E8%87%AA%E7%94%B1%E6%8F%8F%E7%94%BB%E3%82%92%E3%81%99%E3%82%8B%EF%BC%88%E5%9F%BA%E6%9C%AC%EF%BC%89

			// 文字単位で描画を制御するためのループ
			for (const auto& glyph : font.getGlyphs(str))
			{
				// 改行文字なら
				if (glyph.codePoint == U'\n')
				{
					// ペンの X 座標をリセット
					penPos.x = basePos.x;

					// ペンの Y 座標をフォントの高さ分進める
					penPos.y += font.height();

					continue;
				}

				// 文字のテクスチャをペンの位置に文字ごとのオフセットを加算して描画
				// FontMethod がビットマップ方式の場合に限り、Math::Round() で整数座標にすると品質が向上
				glyph.texture.draw(Math::Round(penPos + glyph.getOffset()), col);

				// ペンの X 座標を文字の幅の分進める
				penPos.x += glyph.xAdvance;
			}

		}

	}

}
