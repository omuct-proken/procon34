#include "tab_menu.hpp"

namespace Procon34 {

	namespace Gui {

		TabMenu::TabMenu(
			Array<String> textOnSelector,
			Optional<Font> font
		)
			: m_text(textOnSelector)
			, m_pointer(0)
			, m_font(font.value_or(SimpleGUI::GetFont()))
		{
			recalcExpectSize();
		}

		size_t TabMenu::getValue() const { return m_pointer; }

		StringView TabMenu::getSelectedText() const { return m_text[m_pointer]; }

		RectF TabMenu::getRegionFromGlobalRect(RectF globalRect) {
			double height = Min(m_expectHeight, globalRect.h);
			RectF res = globalRect;
			res.h = height;
			return res;
		}

		Array<RectF> TabMenu::getAreaList(RectF globalRect) {
			Array<RectF> res;

			Vec2 globalSize = globalRect.size;
			double b = Min(m_expectHeight, globalRect.h);
			double w = 0.0;
			for (size_t i = 0; i < m_text.size(); i++) {
				Vec2 tl = globalRect.tl() + Vec2(Min(globalRect.w, w), 0.0);
				Vec2 br = globalRect.tl() + Vec2(Min(globalRect.w, w + m_expectWidth[i]), b);
				RectF thisRect = RectF(tl, br - tl);
				res.push_back(thisRect);

				w += m_expectWidth[i];
			}

			return res;
		}

		void TabMenu::update(RectF globalRect) {
			auto areaList = getAreaList(globalRect);

			for (size_t i = 0; i < m_text.size(); i++) {
				if (areaList[i].leftClicked()) {
					m_pointer = i;
				}
			}
		}

		void TabMenu::draw(RectF globalRect) {
			auto areaList = getAreaList(globalRect);

			for (size_t i = 0; i < m_text.size(); i++) {

				if (m_pointer == i) {
					areaList[i].draw(Palette::Yellow.withAlpha(40));
				}

				areaList[i].drawFrame();

				Vec2 cent = areaList[i].center();
				m_font(m_text[i]).drawAt(cent);
			}
		}

		void TabMenu::recalcExpectSize() {
			m_expectWidth = m_text.map([&](StringView s) -> double {
				return m_font(s).region(Vec2(0.0, 0.0)).w + 10.0;
			});
			m_expectHeight = m_font.height() + 10.0;
		}

	}

}
