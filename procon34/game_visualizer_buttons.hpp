#pragma once
#include "stdafx.h"

namespace Procon34 {

	// クリックすると一時的に色が付くボタン
	class FlashingButton {
	public:
		FlashingButton();
		FlashingButton(std::function<void(RectF)> drawer);
		void trigger();
		void update();
		void draw();
		void setRect(RectF rect);
		void setLightingColor(Optional<Color> color, Optional<double> t = none);
		void setBackgroundColor(Optional<Color> color);
		void resetLighting();
		RectF getRect();
		bool clicked() const;
	private:
		std::function<void(RectF)> m_drawer;
		RectF m_rect;
		double m_lighting = 0.0;
		Optional<Color> m_lightingColor;
		Optional<Color> m_backgroundColor;
		bool m_clicked = false;
		bool m_triggered = false;
	};

	// 8 方向への移動を入力できるボタン
	class FlickButton {
	public:
		FlickButton();
		FlickButton(std::function<void(RectF)> drawer);
		void update();
		void draw();
		void setRect(RectF rect);
		void setLightingColor(Optional<Color> color, Optional<double> t = none);
		void setBackgroundColor(Optional<Color> color);
		void resetLighting();
		RectF getRect();
		bool isTriggered() const;
		int32 getDirection() const;
		void setEnabledDirection(int32 val); // 移動できる向きを下位 9 ビットで指定する
	private:
		void updateCursorPos();

		std::function<void(RectF)> m_drawer;
		RectF m_rect;
		double m_lighting = 0.0;
		Optional<Color> m_lightingColor;
		Optional<Color> m_backgroundColor;
		int32 m_enabledDirection = 0b0'11111111;
		int32 m_direction = 8;
		bool m_holding = false;
		bool m_triggered = false;

		static constexpr double MapAngleToSegment(double angle, double h, double dist) {
			if (dist <= 0.0) return h <= 0;
			double s = Math::Sin(angle);
			double c = Math::Abs(Math::Cos(angle));
			if (c == 0.0) return 0.0;
			return Math::Clamp(s / c, -h, h);
		}
		static constexpr Vec2 MapAngleToRect(double angle, RectF rect) {
			return Vec2(
				MapAngleToSegment(angle, rect.w / 2.0, rect.h / 2.0),
				MapAngleToSegment(angle, rect.h / 2.0, rect.w / 2.0)
			).movedBy(rect.center());
		}
	};

}
