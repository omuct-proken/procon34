#include "request.hpp"


namespace Procon34 {


	Optional<MatchOverview> MatchOverview::FromJson(JSON val) {
		MatchOverview res;
		try {
			res.id = val[U"id"].get<int64>();
			res.turns = val[U"turns"].get<int32>();
			res.turnSeconds = val[U"turnSeconds"].get<int32>();
			res.opponent = val[U"opponent"].getString();
			res.bonusWall = val[U"bonus"][U"wall"].get<int32>();
			res.bonusTerritory = val[U"bonus"][U"territory"].get<int32>();
			res.bonusCastle = val[U"bonus"][U"castle"].get<int32>();
			res.first = val[U"first"].get<bool>();

			JSON board = val[U"board"];
			res.boardWidth = board[U"width"].get<int32>();
			res.boardHeight = board[U"height"].get<int32>();
			res.boardMason = board[U"mason"].get<int32>();

			res.myAgentPos.assign(res.boardMason, BoardPos(0, 0));
			res.opponentAgentPos.assign(res.boardMason, BoardPos(0, 0));

			JSON masonsBoard = board[U"masons"];
			for (int32 r = 0; r < res.boardHeight; r++) {
				for (int32 c = 0; c < res.boardWidth; c++) {
					int32 masonsVal = masonsBoard[r][c].get<int32>();
					if (masonsVal > 0) {
						res.myAgentPos[masonsVal - 1] = BoardPos(r, c);
					}
					else if (masonsVal < 0) {
						res.opponentAgentPos[-masonsVal - 1] = BoardPos(r, c);
					}
				}
			}

		}
		catch (const Error&) {
			return none;
		}
		return res;
	}




	const String token = U"token";
	const URL host = U"http://localhost:3000";
	const HashTable<String, String> header = { { U"procon-token", U"{}"_fmt(token)}};
	const HashTable<String, String> headerForPost = { { U"procon-token", U"{}"_fmt(token)}, { U"Content-Type", U"application/json" } };
	const FilePath path = U"temp.json";

	// [GET] /matches
	Optional<JSON> getMatchesList() {
		const URL endpoint = U"/matches";

		// try
		if (const auto response = SimpleHTTP::Get(host + endpoint, header, path)) {
			if (response.isOK()) {
				const JSON data = JSON::Load(path);
				return data;
			}
			return none; // 漏れてた
		}

		else {
			Console << U"Bad request";
			return none;
		}
	}

	// [GET] /matches/{id}
	Optional<JSON> getMatchesState(int64 id) {
		const URL endpoint = U"/matches/{}"_fmt(id);

		// try
		if (const auto response = SimpleHTTP::Get(host + endpoint, header, path)) {
			if (response.isOK()) {
				const JSON data = JSON::Load(path);
				return data;
			}
			return none;
		}

		else {
			Console << U"Bad request";
			return none;
		}
	}

	// [POST] /matches/{id}
	void postAgentActions(int64 id, JSON json) {
		const URL endpoint = U"/matches/{}"_fmt(id);
		// try
		std::string buf = json.formatUTF8();
		if (auto response = SimpleHTTP::Post(host + endpoint, headerForPost, buf.data(), buf.size(), path)) {
			if (response.isOK()) {
				JSON data = JSON::Load(path);
				Console << U"Request is accepted at {}"_fmt(data[U"accepted_at"].get<uint64>()); // Unix Time が帰ってくるが、可読性を考えるとどうにか変換したほうがよいか？
			}
			else {
				Console << U"Request is NOT accepted";
			}
		}

		else Console << U"Bad request";
	}
}
