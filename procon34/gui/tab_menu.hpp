#pragma once
#include "../stdafx.h"

namespace Procon34 {

	namespace Gui {

		class TabMenu {
		public:

			TabMenu(
				Array<String> textOnSelector,
				Optional<Font> font = none
			);

			size_t getValue() const;

			StringView getSelectedText() const;

			RectF getRegionFromGlobalRect(RectF globalRect);

			Array<RectF> getAreaList(RectF globalRect);

			void update(RectF globalRect);

			void draw(RectF globalRect);

		private:

			Font m_font;
			Array<String> m_text;
			Array<double> m_expectWidth;
			double m_expectHeight;
			size_t m_pointer;

			void recalcExpectSize();

		};

	}

}
