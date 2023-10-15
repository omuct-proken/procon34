#include "stdafx.h"
#include "main_display.hpp"

void Main() {
	Scene::SetBackground(ColorF{ 0.0, 0.0, 0.0 });
	Window::Resize(1280, 720);
	using namespace Procon34;
	MainDisplay mainDisplay;

	while (System::Update()) {
		RectF globalRect = Scene::Rect();
		mainDisplay.update(globalRect);
	}
}
