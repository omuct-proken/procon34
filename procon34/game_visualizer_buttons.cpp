#include "game_visualizer_buttons.hpp"

namespace Procon34 {

	FlashingButton::FlashingButton()
		: m_drawer([](RectF) {})
		, m_rect(RectF(0.0, 0.0, 0.0, 0.0))
	{}

	FlashingButton::FlashingButton(std::function<void(RectF)> drawer)
		: m_drawer(std::move(drawer))
		, m_rect(RectF(0.0, 0.0, 0.0, 0.0))
	{}

	void FlashingButton::trigger() {
		m_lighting = 1.0;
		m_clicked = true;
	}

	void FlashingButton::update() {
		m_clicked = false;
		m_lighting = Max(0.0, m_lighting - Scene::DeltaTime());
		if (m_rect.leftClicked() || m_triggered) {
			trigger();
		}
		m_triggered = false;
	}

	void FlashingButton::draw() {
		if (m_backgroundColor.has_value()) {
			m_rect.draw(m_backgroundColor.value());
		}
		if (m_lightingColor.has_value()) {
			double lighting = Clamp(m_lighting, 0.0, 1.0) * 255.0;
			m_rect.draw(m_lightingColor->withAlpha((int32)lighting));
		}
		m_drawer(m_rect);
	}

	void FlashingButton::setRect(RectF rect) {
		m_rect = rect;
	}

	void FlashingButton::setLightingColor(Optional<Color> color, Optional<double> t) {
		m_lightingColor = color;
		if (t.has_value()) m_lighting = t.value();
	}

	void FlashingButton::setBackgroundColor(Optional<Color> color) {
		m_backgroundColor = color;
	}

	void FlashingButton::resetLighting() {
		m_lighting = 0.0;
	}

	RectF FlashingButton::getRect() { return m_rect; }
	bool FlashingButton::clicked() const { return m_clicked; }




	FlickButton::FlickButton()
		: m_drawer([](RectF) {})
		, m_rect(RectF(0.0, 0.0, 0.0, 0.0))
	{}

	FlickButton::FlickButton(std::function<void(RectF)> drawer)
		: m_drawer(std::move(drawer))
		, m_rect(RectF(0.0, 0.0, 0.0, 0.0))
	{}

	void FlickButton::update() {
		m_triggered = false;

		m_lighting = Max(0.0, m_lighting - Scene::DeltaTime());
		if (m_holding) {
			updateCursorPos();
			auto rect = getRect();
			auto pos = rect.center();
			if (MouseL.up()) {
				if ((m_enabledDirection >> m_direction) & 1) {
					m_lighting = 1.0;
					m_triggered = true;
				}
				m_holding = false;
			}
		}
		if (m_rect.leftClicked()) {
			m_holding = true;
		}
	}

	void FlickButton::draw() {
		if (m_backgroundColor.has_value()) {
			m_rect.draw(m_backgroundColor.value());
		}
		if (m_lightingColor.has_value()) {
			double lighting = Clamp(m_lighting, 0.0, 1.0) * 255.0;
			m_rect.draw(m_lightingColor->withAlpha((int32)lighting));
		}

		double radius = Math::Sqrt(m_rect.h * m_rect.w) * 0.65;
		double inradius = Math::Sqrt(m_rect.h * m_rect.w) * 0.45;
		Vec2 cent = m_rect.center();

		if (m_holding) {
			cent.asCircle(inradius).drawArc(0.0_deg, 360.0_deg, 0.0, radius - inradius, Palette::Black);
		}

		if (m_holding && ((m_enabledDirection >> m_direction) & 1)) {
			if (m_direction == 8) {
				cent.asCircle(inradius).draw(Palette::Red.withAlpha(128));
			}
			else {
				int dir = 2 - m_direction;
				cent.asCircle(inradius).drawArc(45.0_deg * (-0.5 + dir), 45.0_deg, 0.0, radius - inradius, Palette::Red);
			}
		}

		if (m_holding) {
			for (int d = 0; d <= 8; d++) if (!(((m_enabledDirection >> d) & 1))) {
				if (d == 8) {
					cent.asCircle(inradius).draw(Palette::Gray.withAlpha(128));
				}
				else {
					int dir = 2 - d;
					cent.asCircle(inradius).drawArc(45.0_deg * (-0.5 + dir), 45.0_deg, 0.0, radius - inradius, Palette::Gray);
				}
			}
			for (int i = 0; i < 8; i++) {
				Vec2 inP = Vec2(Math::Cos(45.0_deg * (i + 0.5)), Math::Sin(45.0_deg * (i + 0.5)));
				Line(inP * inradius, inP * radius).movedBy(cent).draw(2.0);
				cent.asCircle(radius).drawArc(0.0_deg, 360.0_deg, 0.0, 2.0, Palette::White);
				cent.asCircle(inradius).drawArc(0.0_deg, 360.0_deg, 2.0, 0.0, Palette::White);
			}
		}

		m_drawer(m_rect);
	}

	void FlickButton::setRect(RectF rect) {
		m_rect = rect;
	}

	void FlickButton::setLightingColor(Optional<Color> color, Optional<double> t) {
		m_lightingColor = color;
		if (t.has_value()) m_lighting = t.value();
	}

	void FlickButton::setBackgroundColor(Optional<Color> color) {
		m_backgroundColor = color;
	}

	void FlickButton::resetLighting() {
		m_lighting = 0.0;
	}

	RectF FlickButton::getRect() { return m_rect; }
	bool FlickButton::isTriggered() const { return m_triggered; }
	int32 FlickButton::getDirection() const { return m_direction; }
	void FlickButton::setEnabledDirection(int32 val) { m_enabledDirection = val; }

	void FlickButton::updateCursorPos() {
		auto cent = m_rect.center();
		auto mousePos = Cursor::PosF() - cent;
		if (m_rect.mouseOver() || mousePos.lengthSq() <= 9.0) {
			m_direction = 8;
			return;
		}
		int32 q = static_cast<int32>(Math::Floor((90.0 - Math::ToDegrees(mousePos.getAngle())) / 45.0 + 0.5));
		m_direction = (q + 32) % 8;
	}


}
