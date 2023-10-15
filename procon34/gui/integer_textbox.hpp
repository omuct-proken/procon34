#include "../stdafx.h"

namespace Procon34 {

	namespace Gui {

		class IntegerTextBox {
		public:

			IntegerTextBox(
				StringView name = U"noname",
				int64 limit_min = INT64_MIN,
				int64 limit_max = INT64_MAX,
				Optional<int64> val_default = none
			);

			// 値の範囲を制限する。
			// 必ず設定する。
			void limit(int64 lim_min, int64 lim_max);

			// SimpleGUI::TextBox を呼び出す
			// 更新も描画もこのメソッドに含まれる
			void update(RectF rect, double padding = 3.0);

			bool isOk() const;

			int64 value() const;

			void overwriteValue(int64 val);

			String getErrorMessage() const;

		private:

			TextEditState m_editState;
			int64 m_result = 0;

			// 0 : 成功
			// 1 : 整数ではない
			// 2 : 小さすぎる
			// 3 : 大きすぎる
			int64 m_ok;

			int64 m_limit_min;
			int64 m_limit_max;

			String m_name;

			bool m_need_update; // 入力値が TextBox GUI を通さず行われた場合

			static constexpr Color COLOR_SUCCESS = Palette::Cyan.withAlpha(0);
			static constexpr Color COLOR_FAIL = Palette::Orange;

		};

	}

}
