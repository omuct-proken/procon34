#pragma once

#include "stdafx.h"


namespace Procon34 {

	class MainDisplayImpl;


	class MainDisplay {
	public:

		MainDisplay();

		void update(RectF rect);

	private:

		std::shared_ptr<MainDisplayImpl> impl;

	};

}
