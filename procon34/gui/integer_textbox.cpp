#include "integer_textbox.hpp"

namespace Procon34 {

	namespace Gui {


		IntegerTextBox::IntegerTextBox(
			StringView name,
			int64 limit_min,
			int64 limit_max,
			Optional<int64> val_default
		)
			: m_name(name)
			, m_limit_min(limit_min)
			, m_limit_max(limit_max)
			, m_ok(1)
			, m_need_update(false)
		{
			if (val_default.has_value()) {
				int64 defv = val_default.value();
				if (m_limit_min <= defv && defv <= m_limit_max) {
					m_result = defv;
					m_ok = 0;
					m_editState.text = ToString(defv);
				}
			}
		}

		// 値の範囲を制限する。
		// 必ず設定する。
		void IntegerTextBox::limit(int64 lim_min, int64 lim_max) {
			m_limit_min = lim_min;
			m_limit_max = lim_max;
		}

		// SimpleGUI::TextBox を呼び出す
		// 更新も描画もこのメソッドに含まれる
		void IntegerTextBox::update(RectF rect, double padding) {

			Vec2 paddingSz = Vec2(padding, padding);

			RectF textBoxRect = SimpleGUI::TextBoxRegion(rect.tl() + paddingSz, rect.w - padding * 2);
			RectF lightingRect = RectF(textBoxRect.tl() - paddingSz, textBoxRect.size + paddingSz * 2);

			if (isOk()) {
				lightingRect.draw(COLOR_SUCCESS);
			}
			else {
				lightingRect.draw(COLOR_FAIL);
			}

			if (SimpleGUI::TextBox(m_editState, rect.tl() + Vec2(padding, padding), rect.w - padding * 2) || m_need_update) {

				String text = m_editState.text;
				auto parseResult = ParseIntOpt<int64>(text, Arg::radix(10));

				if (parseResult.has_value()) {
					m_result = parseResult.value();
				}

				if (!parseResult.has_value()) {
					m_ok = 1;
				}
				else if (parseResult.value() < m_limit_min) {
					m_ok = 2;
				}
				else if (m_limit_max < parseResult.value()) {
					m_ok = 3;
				}
				else {
					m_ok = 0;
				}

			}

			m_need_update = false;

		}

		bool IntegerTextBox::isOk() const { return m_ok == 0; }

		int64 IntegerTextBox::value() const { return m_result; }

		void IntegerTextBox::overwriteValue(int64 val) {
			val = Clamp(val, m_limit_min, m_limit_max);
			m_editState.text = ToString(val);
			m_need_update = true;
		}

		String IntegerTextBox::getErrorMessage() const {
			switch (m_ok) {
			case 1:
				return U"「{}」の入力が受理されませんでした。"_fmt(m_name);
			case 2:
				return U"「{}」の値が小さすぎます。（最小値 = {} , 入力 = {}）"_fmt(m_name, m_limit_min, m_result);
			case 3:
				return U"「{}」の値が大きすぎます。（最大値 = {} , 入力 = {}）"_fmt(m_name, m_limit_max, m_result);
			}
			return U"";
		}

	}

}
