#pragma once
#include "../stdafx.h"

namespace Procon34 {

	namespace Gui {

		// Siv3D リファレンス v0.6.10 Chapter 52 サンプル集 | UI
		// プルダウンメニュー
		// https://zenn.dev/reputeless/books/siv3d-documentation/viewer/sample-ui#%E3%83%97%E3%83%AB%E3%83%80%E3%82%A6%E3%83%B3%E3%83%A1%E3%83%8B%E3%83%A5%E3%83%BC

		class Pulldown
		{
		public:

			Pulldown() = default;

			Pulldown(const Array<String>& items, const Font& font, const Point& pos = { 0,0 });

			bool isEmpty() const;

			void update();

			void draw() const;

			void setPos(const Point& pos);

			const Rect& getRect() const;

			size_t getIndex() const;

			String getItem() const;

		private:

			Font m_font;

			Array<String> m_items;

			size_t m_index = 0;

			Size m_padding{ 6, 2 };

			Rect m_rect;

			int32 m_downButtonSize = 16;

			bool m_isOpen = false;
		};

	}

}
