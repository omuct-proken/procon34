#include "pulldown.hpp"

namespace Procon34 {

	namespace Gui {

		// Siv3D リファレンス v0.6.10 Chapter 52 サンプル集 | UI
		// プルダウンメニュー
		// https://zenn.dev/reputeless/books/siv3d-documentation/viewer/sample-ui#%E3%83%97%E3%83%AB%E3%83%80%E3%82%A6%E3%83%B3%E3%83%A1%E3%83%8B%E3%83%A5%E3%83%BC

		Pulldown::Pulldown(const Array<String>& items, const Font& font, const Point& pos)
			: m_font{ font }
			, m_items{ items }
			, m_rect{ pos, 0, (m_font.height() + m_padding.y * 2) }
		{
			for (const auto& item : m_items)
			{
				m_rect.w = Max(m_rect.w, static_cast<int32>(m_font(item).region().w));
			}

			m_rect.w += (m_padding.x * 2 + m_downButtonSize);
		}

		bool Pulldown::isEmpty() const
		{
			return m_items.empty();
		}

		void Pulldown::update()
		{
			if (isEmpty())
			{
				return;
			}

			if (m_rect.leftClicked())
			{
				m_isOpen = (not m_isOpen);
			}

			Point pos = m_rect.pos.movedBy(0, m_rect.h);

			if (m_isOpen)
			{
				for (auto i : step(m_items.size()))
				{
					if (const Rect rect{ pos, m_rect.w, m_rect.h };
						rect.leftClicked())
					{
						m_index = i;
						m_isOpen = false;
						break;
					}

					pos.y += m_rect.h;
				}
			}
		}

		void Pulldown::draw() const
		{
			m_rect.draw();

			if (isEmpty())
			{
				return;
			}

			m_rect.drawFrame(1, 0, m_isOpen ? Palette::Orange : Palette::Gray);

			Point pos = m_rect.pos;

			m_font(m_items[m_index]).draw(pos + m_padding, Palette::Black);

			Triangle{ (m_rect.x + m_rect.w - m_downButtonSize / 2.0 - m_padding.x), (m_rect.y + m_rect.h / 2.0),
				(m_downButtonSize * 0.5), 180_deg }.draw(Palette::Black);

			pos.y += m_rect.h;

			if (m_isOpen)
			{
				const Rect backRect{ pos, m_rect.w, (m_rect.h * m_items.size()) };

				backRect.drawShadow({ 1, 1 }, 4, 1).draw();

				for (const auto& item : m_items)
				{
					if (const Rect rect{ pos, m_rect.size };
						rect.mouseOver())
					{
						rect.draw(Palette::Skyblue);
					}

					m_font(item).draw((pos + m_padding), Palette::Black);

					pos.y += m_rect.h;
				}

				backRect.drawFrame(1, 0, Palette::Gray);
			}
		}

		void Pulldown::setPos(const Point& pos)
		{
			m_rect.setPos(pos);
		}

		const Rect& Pulldown::getRect() const
		{
			return m_rect;
		}

		size_t Pulldown::getIndex() const
		{
			return m_index;
		}

		String Pulldown::getItem() const
		{
			if (isEmpty())
			{
				return{};
			}

			return m_items[m_index];
		}

	}

}
