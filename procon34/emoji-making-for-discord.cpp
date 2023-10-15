
#include "game_visualizer.hpp"


namespace Procon34 {

	inline void EmojiMakingForDiscord()
	{

		{
			Image image(128, 128, Palette::Black);
			auto rect = RectF(Vec2(0.0, 0.0), image.size());
			auto cent = rect.center();
			rect.scaledAt(rect.center(), 0.9).paint(image, Palette::Lime.withAlpha(100));
			Shape2D::Star(rect.h * 0.5 * 0.6).asPolygon().movedBy(cent).paint(image, Palette::White);
			image.savePNG(U"castle.png");
		}

		Image img;

		{
			Image image(128, 128, Palette::Black.withAlpha(0));
			auto rect = RectF(Vec2(0.0, 0.0), image.size());
			auto cent = rect.center();
			auto hexagon = Shape2D::Hexagon(0.1).asPolygon().scaled(rect.h * 0.5).movedBy(cent);
			hexagon.calculateBuffer(45.0).overwrite(image, Palette::White);
			hexagon.calculateBuffer(42.0).overwrite(image, Palette::Red);
			hexagon.calculateBuffer(30.0).overwrite(image, Palette::White);
			hexagon.calculateBuffer(27.0).overwrite(image, Palette::Black.withAlpha(0));
			image.savePNG(U"redagent.png");
			img = image;
		}

		{
			Image image(128, 128, Palette::Black.withAlpha(0));
			auto rect = RectF(Vec2(0.0, 0.0), image.size());
			auto cent = rect.center();
			auto hexagon = Shape2D::Hexagon(0.1).asPolygon().scaled(rect.h * 0.5).movedBy(cent);
			hexagon.calculateBuffer(45.0).overwrite(image, Palette::White);
			hexagon.calculateBuffer(42.0).overwrite(image, Palette::Blue);
			hexagon.calculateBuffer(30.0).overwrite(image, Palette::White);
			hexagon.calculateBuffer(27.0).overwrite(image, Palette::Black.withAlpha(0));
			image.savePNG(U"blueagent.png");
			img = image;
		}

		Texture tex[4];

		using namespace Procon34;
		{
			Image image(128, 128, Palette::Black.withAlpha(0));
			auto rect = RectF(Vec2(0.0, 0.0), image.size());
			auto cent = rect.center();
			rect.scaledAt(cent, 0.8).asPolygon().calculateRoundBuffer(rect.h * 0.1).overwrite(image, Palette::Darkcyan);
			GameControllerVisualizer::GetPlusIcon(rect).paint(image, Palette::White);
			image.savePNG(U"plus_inst.png");
			img = image;
		}

		tex[0] = Texture(img);
		{
			Image image(128, 128, Palette::Black.withAlpha(0));
			auto rect = RectF(Vec2(0.0, 0.0), image.size());
			auto cent = rect.center();
			rect.scaledAt(cent, 0.8).asPolygon().calculateRoundBuffer(rect.h * 0.1).overwrite(image, Palette::Darkcyan);
			GameControllerVisualizer::GetMinusIcon(rect).paint(image, Palette::White);
			image.savePNG(U"minus_inst.png");
			img = image;
		}
		tex[1] = Texture(img);
		{
			Image image(128, 128, Palette::Black.withAlpha(0));
			auto rect = RectF(Vec2(0.0, 0.0), image.size());
			auto cent = rect.center();
			rect.scaledAt(cent, 0.8).asPolygon().calculateRoundBuffer(rect.h * 0.1).overwrite(image, Palette::Darkcyan);
			GameControllerVisualizer::GetCrossIcon(rect).paint(image, Palette::White);
			image.savePNG(U"cross_inst.png");
			img = image;
		}
		tex[2] = Texture(img);
		{
			Image image(128, 128, Palette::Black.withAlpha(0));
			auto rect = RectF(Vec2(0.0, 0.0), image.size());
			auto cent = rect.center();
			rect.scaledAt(cent, 0.8).asPolygon().calculateRoundBuffer(rect.h * 0.1).overwrite(image, Palette::Darkcyan);
			GameControllerVisualizer::GetCheckIcon(rect).paint(image, Palette::White);
			image.savePNG(U"check_inst.png");
			img = image;
		}
		tex[3] = Texture(img);

	}

}
