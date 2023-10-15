
#include "game_state.hpp"
#include "match_viewer.hpp"
#include "module_visualize/ver_01.hpp"
#include "game_visualizer.hpp"



namespace Procon34 {



	namespace {

		struct GameStateTextInfo {
			const Array<Color> teamcolors = {
				Palette::Pink,
				Palette::Skyblue,
				Palette::Yellow
			};

			int32 whosTurn;
			int32 turnIndex;
			int32 allTurnCount;
			int32 castleCoefficient;
			int32 wallCoefficient;
			int32 teritorryCoefficient;
			EachPlayer<int64> score;
			bool isOver;

			static GameStateTextInfo FromGameState(std::shared_ptr<const GameState> gameState) {
				GameStateTextInfo res;

				res.whosTurn = 2;
				if (!gameState->isOver()) {
					if (gameState->whosTurn() == PlayerColor::Red) res.whosTurn = 0; else res.whosTurn = 1;
				}

				res.turnIndex = gameState->getTurnIndex();
				res.allTurnCount = gameState->getAllTurnNumber();

				res.score[PlayerColor::Red] = gameState->getScore(PlayerColor::Red);
				res.score[PlayerColor::Blue] = gameState->getScore(PlayerColor::Blue);

				auto initialState = gameState->getInitialState();
				res.castleCoefficient = initialState->castleCoefficient;
				res.wallCoefficient = initialState->wallCoefficient;
				res.teritorryCoefficient = initialState->teritorryCoefficient;

				res.isOver = gameState->isOver();

				return res;
			}

			void render(Font font, Vec2 leftTop) {

				double fonth = font.height();

				RenderTextColored(
					font,
					{
						{ U"Turn ", Palette::White },
						{ U"#{}"_fmt(turnIndex), teamcolors[whosTurn] },
						{ U" / {}"_fmt(allTurnCount), Palette::White }
					},
					leftTop
				);

				double offsety = 50.0;
				font(U"Score :").draw(leftTop + Vec2(0.0, offsety + (fonth + 5.0) / 2.0), Palette::White);
				font(U"{}"_fmt(score[PlayerColor::Red])).draw(leftTop + Vec2(120.0, offsety), teamcolors[0]);
				font(U"{}"_fmt(score[PlayerColor::Blue])).draw(leftTop + Vec2(120.0, offsety + fonth + 5.0), teamcolors[1]);
				offsety += (fonth + 5.0) * 2.0;

				font(U"城：{}"_fmt(castleCoefficient)).draw(leftTop + Vec2(0.0, offsety), Palette::White);
				offsety += (fonth + 5.0);
				font(U"壁：{}"_fmt(wallCoefficient)).draw(leftTop + Vec2(0.0, offsety), Palette::White);
				offsety += (fonth + 5.0);
				font(U"陣：{}"_fmt(teritorryCoefficient)).draw(leftTop + Vec2(0.0, offsety), Palette::White);
				offsety += (fonth + 5.0);

				offsety += (fonth + 5.0);

				if (isOver) {
					font(U"Game Over").draw(leftTop + Vec2(0.0, offsety), Palette::Orange);
				}
			}

		};

	} // anonymous namespace

	namespace ViewerNS {

		struct StateDigest {
			static constexpr int32 BiomeNormal = 1 << 0;
			static constexpr int32 BiomeCastle = 1 << 1;
			static constexpr int32 BiomePond = 1 << 2;
			static constexpr int32 RedWall = 1 << 3;
			static constexpr int32 BlueWall = 1 << 4;
			static constexpr int32 TerritoryRed = 1 << 5;
			static constexpr int32 TerritoryBlue = 1 << 6;
			static constexpr int32 ClosedTerritoryRed = 1 << 7;
			static constexpr int32 ClosedTerritoryBlue = 1 << 8;

			Grid<int32> grid;
			EachPlayer<Array<BoardPos>> agents;

			GameStateTextInfo textInfo;
			EachPlayer<int64> score;

			static MassBiome MaskToBiome(int32 mask) {
				if (mask & BiomePond) return MassBiome::Pond;
				if (mask & BiomeCastle) return MassBiome::Castle;
				return MassBiome::Normal;
			}

			static StateDigest LoadFromGameState(BoxPtr<const GameState> game) {
				auto board = game->getBoard();
				auto grid = Grid<int32>(board->getSize(), 0);

				// grid を構築する
				for (int r = 0; r < (int)grid.height(); r++) {
					for (int c = 0; c < (int)grid.width(); c++) {
						auto pos = BoardPos(r, c);
						auto mass = board->operator[](pos);
						if (mass.biome == MassBiome::Normal) grid[pos.asPoint()] |= BiomeNormal;
						if (mass.biome == MassBiome::Castle) grid[pos.asPoint()] |= BiomeCastle;
						if (mass.biome == MassBiome::Pond) grid[pos.asPoint()] |= BiomePond;
						if (mass.wall.has_value()) {
							if (mass.wall->color == PlayerColor::Red) grid[pos.asPoint()] |= RedWall;
							if (mass.wall->color == PlayerColor::Blue) grid[pos.asPoint()] |= BlueWall;
						}
						if (game->isAreaOf(PlayerColor::Red, pos)) {
							grid[pos.asPoint()] |= TerritoryRed;
						}
						if (game->isAreaOf(PlayerColor::Blue, pos)) {
							grid[pos.asPoint()] |= TerritoryBlue;
						}
					}
				}

				// agents を構築する
				EachPlayer<Array<BoardPos>> agents;
				agents[PlayerColor::Red] = game->getAgents(PlayerColor::Red).map([](Agent a) { return a.pos; });
				agents[PlayerColor::Blue] = game->getAgents(PlayerColor::Blue).map([](Agent a) { return a.pos; });

				// score を構築する
				EachPlayer<int64> score;
				score[PlayerColor::Red] = game->getScore(PlayerColor::Red);
				score[PlayerColor::Blue] = game->getScore(PlayerColor::Blue);

				auto textInfo = GameStateTextInfo::FromGameState(game);

				return StateDigest{
					.grid = std::move(grid),
					.agents = std::move(agents),
					.textInfo = textInfo,
					.score = score
				};
			}
		};

		struct MatchDigest {
			GameInitialState initial;
			Array<StateDigest> states;
			Array<TurnInstructionRecord> instructions;

			// MatchRecord の情報から、ビューワー内部で使用する情報を取得する。
			static MatchDigest FromRecord(MatchRecord record) {
				auto game = GameState::FromInitialState(record.initialState);
				Array<StateDigest> states;

				for (int turnid = 0; turnid < record.initialState.turnCount; turnid++) {
					states.push_back(StateDigest::LoadFromGameState(game));

					auto turn = game->whosTurn();
					auto agents = game->getAgents(turn);

					TurnInstruction inst(game, turn);
					auto instRecord = record.turns[turnid].instructions;
					for (size_t i = 0; i < instRecord.size() && i < agents.size(); i++) {
						auto mov = AgentMove::GetStay(agents[i]);
						if (instRecord[i].type == U"Move") mov = AgentMove::GetMove(agents[i], instRecord[i].direction.value());
						if (instRecord[i].type == U"Construct") mov = AgentMove::GetConstruct(agents[i], instRecord[i].direction.value());
						if (instRecord[i].type == U"Destroy") mov = AgentMove::GetDestroy(agents[i], instRecord[i].direction.value());
						inst.insert(mov);
					}

					game->makeMove(game->whosTurn(), inst);
				}

				states.push_back(StateDigest::LoadFromGameState(game));

				return MatchDigest{
					.initial = record.initialState,
					.states = std::move(states),
					.instructions = record.turns
				};
			}
		};

	} // namespace ViewerNS


	MatchViewer::MatchViewer(MatchRecord record)
		: m_digest(std::make_shared<ViewerNS::MatchDigest>(ViewerNS::MatchDigest::FromRecord(record)))
	{

	}


	void MatchViewer::drawBoard(RectF rect, int turnIndex) {

		auto& initialState = m_digest->initial;

		Size boardSize = Size(
			initialState.boardWidth,
			initialState.boardHeight
		);

		auto& state = m_digest->states[turnIndex];

		auto drawer = Visualizer_01::BoardDrawer(rect, boardSize);

		for (int r = 0; r < boardSize.y; r++) {
			for (int c = 0; c < boardSize.x; c++) {
				auto pos = BoardPos(r, c);
				auto massdat = state.grid[pos.asPoint()];
				auto biome = ViewerNS::StateDigest::MaskToBiome(massdat);

				drawer.drawBiome(biome, pos);

				if (massdat & ViewerNS::StateDigest::ClosedTerritoryRed) {
					drawer.drawArea(PlayerColor::Red, true, pos);
				}
				else if (massdat & ViewerNS::StateDigest::TerritoryRed) {
					drawer.drawArea(PlayerColor::Red, false, pos);
				}

				if (massdat & ViewerNS::StateDigest::ClosedTerritoryBlue) {
					drawer.drawArea(PlayerColor::Blue, true, pos);
				}
				else if (massdat & ViewerNS::StateDigest::TerritoryBlue) {
					drawer.drawArea(PlayerColor::Blue, false, pos);
				}

				if (massdat & ViewerNS::StateDigest::RedWall) {
					drawer.drawWall(PlayerColor::Red, pos);
				}

				if (massdat & ViewerNS::StateDigest::BlueWall) {
					drawer.drawWall(PlayerColor::Blue, pos);
				}

			}
		}

		for (PlayerColor player : { PlayerColor::Red, PlayerColor::Blue }) {
			for (auto pos : state.agents[player]) {
				drawer.drawAgent(player, pos);
			}
		}
	}


	void MatchViewer::drawBoardAndInfo(RectF rect, int turnIndex, Font font) {
		double textInfoWidth = 400.0;
		Vec2 textGuiLeftTop = rect.tr() + Vec2(-textInfoWidth, 50.0);

		drawBoard(RectF(rect.tl(), rect.size - Vec2(textInfoWidth, 0.0)), turnIndex);

		auto& state = m_digest->states[turnIndex];
		state.textInfo.render(font, textGuiLeftTop);
	}


	int32 MatchViewer::getTurnCount() const {
		return (int32)m_digest->instructions.size();
	}


} // namespace Procon34
