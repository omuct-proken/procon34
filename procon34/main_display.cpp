#include "main_display.hpp"

#include "game_state.hpp"
#include "game_instructions.hpp"
#include "game_visualizer.hpp"

#include "gui/tab_menu.hpp"
#include "gui/pulldown.hpp"

#include "solvers/solver_list.hpp"
#include "module_map_editor/map_editor_01.hpp"
#include "match_viewer.hpp"

#include "request.hpp"


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


	namespace {

		MatchRecord ConstructRecordFromReceivedData(
			MatchOverview overview,
			JSON receivedState
		) {
			auto state = GameState::FromJson(receivedState, overview);
			auto initialState = state->getInitialState();

			MatchRecord res;
			res.initialState = *initialState;
			res.turns.resize(initialState->turnCount);

			AgentInstructionRecord defval;
			defval.type = U"Stay";
			defval.direction = unspecified;
			for (auto& a : res.turns) a.instructions.assign(initialState->agentPos[PlayerColor::Red].size(), defval);

			for (auto turn : receivedState[U"logs"].arrayView()) {
				int32 turnId = turn[U"turn"].get<int32>() - 1;
				int32 i = 0;
				for (auto ag : turn[U"actions"].arrayView()) {
					auto& result = res.turns[turnId].instructions[i];
					if (ag[U"succeeded"].get<bool>()) {
						int32 dir = ag[U"dir"].get<int32>();
						int32 type = ag[U"type"].get<int32>();
						if (type == 1) {
							result.type = U"Move";
							result.direction = MoveDirection((12 - dir) % 8);
						}
						else if (type == 2) {
							result.type = U"Construct";
							result.direction = MoveDirection((12 - dir) % 8);
						}
						else if (type == 3) {
							result.type = U"Destroy";
							result.direction = MoveDirection((12 - dir) % 8);
						}
						else {
							result.type = U"Stay";
							result.direction = unspecified;
						}
					}
					i++;
				}
			}

			return res;
		}

	}


	namespace {

		struct MatchInteractor {
		private:

			const MatchOverview m_matchOverview;
			std::shared_ptr<SolverInterface> m_solver;

			// これより上のメンバは、 readonly として、 mutex で保護されない。

			std::mutex m_mutexForAllInfo;
			int32 m_lastTurnProcessed;
			bool m_messageToEnd;

			std::thread m_solverThread;
			MatchRecord m_record;

			// m_mutexForAllInfo は、これより上のメンバを保護する

			std::mutex m_mutexForErrMsg;
			String m_lastError;


			void run() {
				while (true) {
					// エラーが出たらエラーメッセージを更新して
					//    500 msec 待つ。
					try {
						while (true) {
							{
								auto lck = std::lock_guard(m_mutexForAllInfo);
								if (m_messageToEnd) return;
							}

							JSON matchesIdResponse = getMatchesState(m_matchOverview.id).value_or(JSON());

							int32 nextTurn = matchesIdResponse[U"turn"].get<int32>();

							auto info = getAllInfo();

							if (info.lastTurn != nextTurn) {
								auto lck = std::lock_guard(m_mutexForAllInfo);
								m_record = ConstructRecordFromReceivedData(info.matchOverview, matchesIdResponse);

								if (nextTurn % 2 == (info.matchOverview.first ? 0 : 1) && nextTurn < info.matchOverview.turns) {

									auto gameState = GameState::FromJson(matchesIdResponse, info.matchOverview);
									auto instruction = m_solver->operator()(gameState);

									auto instructionJson = instruction.toJson();
									postAgentActions(info.matchOverview.id, instructionJson);

								}
							}

							{
								auto lck = std::lock_guard(m_mutexForAllInfo);
								m_lastTurnProcessed = nextTurn;
							}
							System::Sleep(500.0ms);
						}
					}
					catch (const Error& e) {
						auto lck = std::lock_guard(m_mutexForErrMsg);
						m_lastError = e.what();
						Console << U"--- ";
						Console << U"!!! ERROR !!!";
						Console << m_lastError;
						Console << U"--- ";
					}
					System::Sleep(500.0ms);
				}
			}

		public:

			MatchInteractor(
				MatchOverview matchOverview,
				std::shared_ptr<SolverInterface> solver
			)
				: m_matchOverview(matchOverview)
				, m_solver(solver)
				, m_lastTurnProcessed(-1)
				, m_messageToEnd(false)
				, m_lastError(U"")
			{
				m_solverThread = std::thread(std::function<void()>([this]() { this->run(); }));
			}

			~MatchInteractor() {
				stop();
			}

			String getErrorMessage() {
				auto lck = std::lock_guard(m_mutexForErrMsg);
				return m_lastError;
			}

			void stop() {
				{
					auto lck = std::lock_guard(m_mutexForAllInfo);
					m_messageToEnd = true;
				}
				m_solverThread.join();
			}

			struct ProgressInfo {
				MatchOverview matchOverview;
				int32 lastTurn;
			};

			ProgressInfo getAllInfo() {
				auto lck = std::lock_guard(m_mutexForAllInfo);
				ProgressInfo res;
				res.matchOverview = m_matchOverview;
				res.lastTurn = m_lastTurnProcessed;
				return res;
			}

			MatchRecord getMatchRecord() {
				auto lck = std::lock_guard(m_mutexForAllInfo);
				return m_record;
			}
		};

	} // namespace
	// for MatchInteractor


	class MainDisplayImpl {


		struct GameControllerGui {

			const Array<Color> teamcolors = {
				Palette::Pink,
				Palette::Skyblue,
				Palette::Yellow
			};

			std::shared_ptr<GameState> gameState;
			std::shared_ptr<GameControllerVisualizer> gameVis;
			Font font;
			bool initialized;

			GameControllerGui()
				: font(30)
				, initialized(false)
			{}

			void initialize(std::shared_ptr<GameState> state) {
				gameState = state;
				gameVis = std::make_shared<GameControllerVisualizer>(gameState, RectF(40.0, 40.0, 720.0, 640.0));

				initialized = true;
			}

			void update(RectF rect) {
				if (!initialized) return;

				Vec2 TextGuiLeftTop = rect.tr() + Vec2(-400.0, 50.0);
				gameVis->setRect(RectF(rect.tl() + Vec2(40.0, 40.0), rect.size + Vec2(-480.0, -80.0)));

				gameVis->update();
			}

			void draw(RectF rect) {
				if (!initialized) return;

				Vec2 TextGuiLeftTop = rect.tr() + Vec2(-400.0, 40.0);
				gameVis->setRect(RectF(rect.tl() + Vec2(40.0, 40.0), rect.size + Vec2(-480.0, -80.0)));

				gameVis->draw();

				int whosTurn = 2;
				if (!gameState->isOver()) {
					if (gameState->whosTurn() == PlayerColor::Red) whosTurn = 0; else whosTurn = 1;
				}

				GameStateTextInfo::FromGameState(gameState).render(font, TextGuiLeftTop);
			}
		};

		struct MapEditorGui {
			std::shared_ptr<MapEditor_01::MapEditor> editor;

			MapEditorGui(std::shared_ptr<MapEditor_01::MapEditor> src = nullptr) {
				if (!src) src = std::make_shared<MapEditor_01::MapEditor>();
				editor = src;
			}

			void update(RectF) {
			}

			void draw(RectF rect) {
				editor->update(rect);
			}
		};

		struct MatchingGui {
			EachPlayer<Array<std::shared_ptr<SolverInterface>>> solvers;
			Gui::Pulldown menu_a;
			Gui::Pulldown menu_b;
			Gui::IntegerTextBox textbox_turnCount;
			Gui::Pulldown menu_field;
			Font font;
			std::shared_ptr<GameSimulator> simulator;
			std::shared_ptr<MatchRecord> recordDest;
			String fieldSearchPath;
			Array<String> fieldFilePaths;

			MatchingGui(
				Font _font
			)
				: textbox_turnCount(U"noname", 2, 1000, 200)
			{
				solvers[PlayerColor::Red] = SolverInterface::ListOfSolvers();
				solvers[PlayerColor::Blue] = SolverInterface::ListOfSolvers();
				auto nameList = solvers[PlayerColor::Red].map([&](std::shared_ptr<SolverInterface> x) { return x->name(); });
				menu_a = Gui::Pulldown(nameList, _font);
				menu_b = Gui::Pulldown(nameList, _font);
				font = _font;
				reloadFiles();
			}

			const Vec2 TLPadding = Vec2(20.0, 20.0);
			const double XOffset1 = 300.0;
			bool startButtonDown = false;

			void reloadFiles() {
				fieldSearchPath = FileSystem::CurrentDirectory() + U"data/field";
				fieldFilePaths = FileSystem::DirectoryContents(fieldSearchPath, Recursive::Yes).map([&](String s) -> String {
					return s.substr(fieldSearchPath.size() + 1);
				}).filter([](String s) { return s.ends_with(U".csv"); });
				menu_field = Gui::Pulldown(fieldFilePaths, font);
			}

			void update(RectF rect) {

				if (startButtonDown) {
					auto initialState = createGameInitialState();
					auto game = GameState::FromInitialState(initialState);
					auto strategy1 = SolverWrapper(solvers[PlayerColor::Red][menu_a.getIndex()]);
					auto strategy2 = SolverWrapper(solvers[PlayerColor::Blue][menu_b.getIndex()]);
					simulator = std::make_shared<GameSimulator>(game, strategy1, strategy2);

					simulator->startSimulating();
					recordDest = std::make_shared<MatchRecord>(simulator->getMatchRecord());
				}

				Vec2 tl = rect.tl() + TLPadding;

				// レイアウト
				{
					Vec2 tl2 = tl;
					menu_a.setPos(Vec2(tl2.x, tl2.y + 35.0).asPoint());
					tl2.y += 80.0;
					menu_b.setPos(Vec2(tl2.x, tl2.y + 35.0).asPoint());
				}
				tl.x += XOffset1;
				{
					Vec2 tl2 = tl;
					menu_field.setPos(Vec2(tl2.x, tl2.y + 35.0).asPoint());
					tl2.y += 80.0;
					// textbox_turnCount.update(RectF(tl2 + Vec2(0.0, 35.0), Vec2(XOffset1 - 20.0, 50.0)));
				}
				tl.x += XOffset1;

				menu_field.update();
				menu_b.update();
				menu_a.update();
			}

			void draw(RectF rect) {

				Vec2 tl = rect.tl() + TLPadding;

				// レイアウト
				{
					Vec2 tl2 = tl;
					font(U"赤陣営").draw(tl2.x, tl2.y + 0.0);
					menu_a.setPos(Vec2(tl2.x, tl2.y + 35.0).asPoint());
					tl2.y += 80.0;
					font(U"青陣営").draw(tl2.x, tl2.y + 0.0);
					menu_b.setPos(Vec2(tl2.x, tl2.y + 35.0).asPoint());
				}
				tl.x += XOffset1;
				{
					Vec2 tl2 = tl;
					font(U"マップ").draw(tl2.x, tl2.y + 0.0);
					menu_field.setPos(Vec2(tl2.x, tl2.y + 35.0).asPoint());
					tl2.y += 80.0;
					font(U"ターン数").draw(tl2.x, tl2.y + 0.0);
					textbox_turnCount.update(RectF(tl2 + Vec2(0.0, 35.0), Vec2(XOffset1 - 20.0, 50.0)));
				}
				tl.x += XOffset1;
				startButtonDown = SimpleGUI::Button(U"スタート", Vec2(tl.x, tl.y), unspecified, !startButtonDown);

				menu_field.draw();
				menu_b.draw();
				menu_a.draw();
			}

			GameInitialState createGameInitialState() {
				GameInitialState res;
				if (!menu_field.isEmpty()) {
					res.agentPos[PlayerColor::Red].clear();
					res.agentPos[PlayerColor::Blue].clear();
					{
						String fieldCsvPath = fieldSearchPath + U"/" + menu_field.getItem();
						auto csv = CSV(fieldCsvPath);
						int32 h = (int32)csv.rows();
						int32 w = (int32)csv.columns(0);
						res.boardHeight = h;
						res.boardWidth = w;
						res.biomeGrid.assign(Size(w, h), MassBiome::Normal);
						for (int32 r = 0; r < h; r++) for (int32 c = 0; c < w; c++) {
							BoardPos pos = BoardPos(r, c);
							Point posp = pos.asPoint();
							String s = csv.get(r, c);
							if (s == U"0") {
								res.biomeGrid[posp] = MassBiome::Normal;
							}
							if (s == U"1") {
								res.biomeGrid[posp] = MassBiome::Pond;
							}
							if (s == U"2") {
								res.biomeGrid[posp] = MassBiome::Castle;
							}
							if (s == U"a") {
								res.biomeGrid[posp] = MassBiome::Normal;
								res.agentPos[PlayerColor::Red].push_back(pos);
							}
							if (s == U"b") {
								res.biomeGrid[posp] = MassBiome::Normal;
								res.agentPos[PlayerColor::Blue].push_back(pos);
							}
						}
					}
					res.turnTimeLimitInMiliseconds = 3000;
					res.firstToMove = PlayerColor::Red;
					res.wallCoefficient = 10;
					res.teritorryCoefficient = 30;
					res.castleCoefficient = 100;
					if (textbox_turnCount.isOk()) {
						res.turnCount = (int32)textbox_turnCount.value();
					}
				}
				return res;
			}

			Optional<std::shared_ptr<MatchRecord>> getLastRecord() {
				if (recordDest) {
					auto buf = recordDest;
					recordDest = nullptr;
					return buf;
				}
				return none;
			}

		};

		struct MatchViewerGui {
			std::shared_ptr<MatchViewer> viewer;
			double sliderPos = 0.0;

			MatchViewerGui() {
			}

			void setRecord(MatchRecord src) {
				viewer = std::make_shared<MatchViewer>(src);
			}

			void update(RectF) {
			}

			void draw(RectF rect, Font font) {
				if (!viewer) return;

				SimpleGUI::SliderAt(sliderPos, rect.bottomCenter() + Vec2(0.0, -50.0), Min(rect.w * 0.6, rect.w - 30.0));

				int32 turnIndex = (int32)Round(sliderPos * viewer->getTurnCount());

				RectF boardRect = RectF(rect.tl() + Vec2(10.0, 10.0), rect.size - Vec2(20.0, 120.0));

				viewer->drawBoardAndInfo(boardRect, turnIndex, font);
			}
		};

		struct MatchInteractorGui {

			const double BasicRowHeight = 45.0;

			Array<Optional<MatchOverview>> matchOverviews;
			Array<int32> insertAwait;
			std::list<std::pair<int32, std::shared_ptr<MatchInteractor>>> interactors; // leave したときのフラグをつける
			Vec2 scrollOffset = Vec2(0.0, 0.0);

			bool reloadButtonClickBuffer = false;
			Array<int32> overviewButtonClicked; // matchOverviews と同じサイズを保つ。

			bool isTheMatchRunning(int64 id) {
				for (auto& [flag, a] : interactors) {
					if (flag == 0 && a->getAllInfo().matchOverview.id == id) {
						return true;
					}
				}
				return false;
			}

			Array<MatchRecord> getAllMatchRecord(){
				Array<MatchRecord> res;
				for (auto& a : interactors) {
					res.push_back(a.second->getMatchRecord());
				}
				return res;
			}

			MatchInteractorGui()
			{
			}

			void update(RectF rect) {
				if (rect.contains(Cursor::PosF())) {
					scrollOffset.y += Mouse::Wheel()* BasicRowHeight;
					scrollOffset.y = Clamp(scrollOffset.y, 0.0, 1000.0);
				}

				// 選択された試合に接続
				for (size_t i = 0; i < matchOverviews.size(); i++) if (overviewButtonClicked[i] != 0) {
					auto solver = Procon34::SolverInterface::ListOfSolvers().back();
					interactors.emplace_back(0, std::make_shared<MatchInteractor>(matchOverviews[i].value(), solver));
				}

				if (reloadButtonClickBuffer) {
					matchOverviews.clear();

					//JSON matchListJson = JSON();
					JSON matchListJson = getMatchesList().value_or(JSON()); // 握りつぶしてるけど、まあ許して…
					if (matchListJson.hasElement(U"matches") && matchListJson[U"matches"].isArray()) {
						for (auto entry : matchListJson[U"matches"].arrayView()) {
							auto parsed = MatchOverview::FromJson(entry);
							matchOverviews.push_back(parsed);
						}
					}

					reloadButtonClickBuffer = false;
				}

				for(auto iter = interactors.begin(); iter != interactors.end(); ){
					auto [flag, interactor] = *iter;
					if (flag != 0) {
						interactor->stop();
						iter = interactors.erase(iter);
					}
					else {
						iter++;
					}
				}

				overviewButtonClicked.assign(matchOverviews.size(), 0);
			}

			void draw(RectF rect, Font font) {
				auto scopedTransformer = Transformer2D(Mat3x2::Identity().translated(rect.tl()), TransformCursor::Yes);
				auto scopedViewport = ScopedViewport2D(Vec2(0.0, 0.0).asPoint(), rect.size.asPoint());

				double leftbound = 30.0;
				double rightbound = rect.w - 30.0;
				Vec2 currentLT = Vec2(leftbound, 30.0);

				auto drawSeparator = [&]() {
					currentLT.x = leftbound;
					currentLT.y += BasicRowHeight * 0.1;
					Line(currentLT, Vec2(rightbound, currentLT.y)).movedBy(-scrollOffset).draw(2.0);
					currentLT.y += BasicRowHeight * 0.1;
					};

				drawSeparator();

				reloadButtonClickBuffer = SimpleGUI::Button(U"Reload", currentLT - scrollOffset);
				currentLT.y += BasicRowHeight;

				for (size_t i = 0; i < matchOverviews.size(); i++) {
					auto& overview = matchOverviews[i];

					if (overview.has_value()) {
						if (isTheMatchRunning(overview->id)) continue;

						double buttonw = SimpleGUI::ButtonRegion(U"Start", Vec2(0.0, 0.0)).w;
						if (SimpleGUI::Button(U"Start", currentLT - scrollOffset)) {
							overviewButtonClicked[i] = 1;
						}

						font(U"id:{} . {} turns . {}sec . {}x{} . {}-{}-{}"_fmt(
							overview->id,
							overview->turns,
							overview->turnSeconds,
							overview->boardHeight,
							overview->boardWidth,
							overview->bonusWall, overview->bonusTerritory, overview->bonusCastle
						)).draw(Arg::leftCenter(currentLT + Vec2(buttonw + 10.0, BasicRowHeight * 0.5) - scrollOffset), Palette::White);
						currentLT.y += BasicRowHeight;

						font(U"vs. {}"_fmt(
							overview->opponent
						)).draw(Arg::leftCenter(currentLT + Vec2(buttonw + 10.0, BasicRowHeight * 0.5) - scrollOffset), Palette::White);
						currentLT.y += BasicRowHeight;
					}
					else {
						font(U"Load Error"_fmt()).draw(Arg::leftCenter(currentLT + Vec2(0.0, BasicRowHeight * 0.5) - scrollOffset), Palette::Orangered);
						currentLT.y += BasicRowHeight;
					}

				}

				drawSeparator();

				for (auto& [flag, interactor] : interactors) if (flag == 0) {
					auto info = interactor->getAllInfo();
					const auto& overview = info.matchOverview;

					double buttonw = SimpleGUI::ButtonRegion(U"Leave", Vec2(0.0, 0.0)).w;
					if (SimpleGUI::Button(U"Leave", currentLT - scrollOffset)) {
						flag = 1;
					}

					font(U"id:{} . #{}/{} turns . {}sec . {}x{} . {}-{}-{}"_fmt(
						overview.id,
						info.lastTurn,
						overview.turns,
						overview.turnSeconds,
						overview.boardHeight,
						overview.boardWidth,
						overview.bonusWall, overview.bonusTerritory, overview.bonusCastle
					)).draw(Arg::leftCenter(currentLT + Vec2(buttonw + 10.0, BasicRowHeight * 0.5) - scrollOffset), Palette::White);
					currentLT.y += BasicRowHeight;

					font(U"vs. {}"_fmt(
						overview.opponent
					)).draw(Arg::leftCenter(currentLT + Vec2(buttonw + 10.0, BasicRowHeight * 0.5) - scrollOffset), Palette::White);
					currentLT.y += BasicRowHeight;

					auto errMsg = interactor->getErrorMessage();

					drawSeparator();

				}
			}
		};


		Font regularFont;
		Font semilargeFont;

		GameInitialState presetInitialState;
		std::shared_ptr<GameState> gameState;
		GameControllerGui gameControllerGui;
		MapEditorGui mapEditorGui;
		MatchingGui matchingGui;
		MatchViewerGui matchViewerGui;
		MatchInteractorGui matchInteractorGui;
		MatchViewerGui ongoingMatchViewerGui;

		Array<DateTime> matchRecordDatetime;
		Array<std::shared_ptr<MatchRecord>> matchRecords;
		Gui::TabMenu globalMenu;

	public:

		MainDisplayImpl()
			: regularFont(20)
			, semilargeFont(30)
			, matchingGui(regularFont)
			, globalMenu({})
		{

			// presetInitialState の内容を検証し、エラーをコンソールに出力
			auto initialStateWrongMessages = presetInitialState.verify(false);
			if (initialStateWrongMessages.has_value()) for (auto& msg : *initialStateWrongMessages) Console << msg;


			gameState = GameState::FromInitialState(presetInitialState);

			gameControllerGui.initialize(gameState);


			globalMenu = Gui::TabMenu(
				{ U"試合状況", U"マップエディタ", U"試合管理", U"鑑賞", U"サーバーと通信", U"通信試合" }
			);
		}

		void update(RectF rect) {


			while (System::Update())
			{

				// メニューのぶんの高さを確保する。
				RectF globalRect = rect;
				auto menuHeight = globalMenu.getRegionFromGlobalRect(globalRect).h;

				Vec2 padding = Vec2(10.0, 10.0);
				RectF contentRect = RectF(globalRect.tl() + Vec2(0.0, menuHeight), globalRect.size - Vec2(0.0, menuHeight));
				contentRect.pos += padding;
				contentRect.size -= padding * 2;

				// メニュー更新処理
				globalMenu.update(globalRect);


				// メニューの選ばれた要素に応じて動作を変える。

				size_t menuValue = globalMenu.getValue();

				if (menuValue == 0) {
					gameControllerGui.update(contentRect);
					gameControllerGui.draw(contentRect);
				}
				if (menuValue == 1) {
					mapEditorGui.update(contentRect);
					mapEditorGui.draw(contentRect);
				}
				if (menuValue == 2) {
					matchingGui.update(contentRect);
					matchingGui.draw(contentRect);
					auto buf = matchingGui.getLastRecord();
					if (buf.has_value()) {
						matchRecordDatetime.push_back(DateTime::Now());
						matchRecords.push_back(buf.value());
						matchViewerGui.setRecord(*(matchRecords.back()));
					}
				}
				if (menuValue == 3) {
					matchViewerGui.update(contentRect);
					matchViewerGui.draw(contentRect, semilargeFont);
				}
				if (menuValue == 4) {
					matchInteractorGui.update(contentRect);
					matchInteractorGui.draw(contentRect, semilargeFont);
				}
				if (menuValue == 5) {
					// 間引く
					if (Scene::FrameCount() % 60 == 0) {
						auto buf = matchInteractorGui.getAllMatchRecord();
						if (!buf.empty()) ongoingMatchViewerGui.setRecord(buf[0]);
						ongoingMatchViewerGui.update(contentRect);
						ongoingMatchViewerGui.draw(contentRect, semilargeFont);
					}
				}

				// メニューは最後に（ほかのものを描いた上に）描画
				globalMenu.draw(globalRect);
			}

		}


	};


	MainDisplay::MainDisplay() : impl(std::make_shared<MainDisplayImpl>()) {}

	void MainDisplay::update(RectF rect) {
		impl->update(rect);
	}

}
