#pragma once
#include "stdafx.h"



namespace Procon34 {

	// エイリアス（非推奨）
	template<class A>
	using BoxPtr = std::shared_ptr<A>;

	enum class PlayerColor {
		Red,
		Blue
	};

	enum class MassBiome {
		Pond,    // 池
		Castle,  // 城
		Normal   // 通常
	};

	// 0 未満または 8 以上の値で初期化するとエラー
	// 高速化したいよりも、ミスしたくないときに使う
	//
	// 右方向が 0　、反時計回りに 45 度回るごとに +1
	class MoveDirection {
	private:
		int32 val;
	public:
		constexpr MoveDirection() : val(0) {}
		constexpr MoveDirection(int32 v) : val(v) {
			if (val < 0 || 8 <= val) throw Error(U"MoveDirection range error");
		}
		MoveDirection ccw45Deg(int32 count) const noexcept { return MoveDirection((val + count) & 7); }
		bool is4Direction() const noexcept { return val % 2 == 0; }
		int32 value() const noexcept { return val; }
		static inline constexpr int32 Right = 0;
		static inline constexpr int32 RightUp = 1;
		static inline constexpr int32 Up = 2;
		static inline constexpr int32 LeftUp = 3;
		static inline constexpr int32 Left = 4;
		static inline constexpr int32 LeftDown = 5;
		static inline constexpr int32 Down = 6;
		static inline constexpr int32 RightDown = 7;

		friend void Formatter(FormatData& formatData, const MoveDirection& value) {
			static const char32* texts[] = { U"Right", U"RightUp", U"Up", U"LeftUp", U"Left", U"LeftDown", U"Down", U"RightDown" };
			return Formatter(formatData, texts[value.value()]);
		}
	};


	// プレイヤーの人数（非推奨）
	// 長さ 2 の配列をつくるときは、 EachPlayer を使うことを検討する
	constexpr int32 NUM_PLAYER = 2;


	// 盤面上のマスを座標で表すときはなるべく BoardPos を使う
	//
	// (r,c)
	//
	//  +-------+-------+-------
	//  | (0,0) | (0,1) | (0,2)
	//  +-------+-------+-------
	//  | (1,0) | (1,1) | (1,2)   ...
	//  +-------+-------+-------
	//  | (2,0) | (2,1) | (2,2)
	//
	//             :
	//
	struct BoardPos {
		int32 r;
		int32 c;

		BoardPos() : r(0), c(0) {}
		BoardPos(int32 _r, int32 _c) : r(_r), c(_c) {}

		BoardPos& moveAlong(MoveDirection direction, int32 steps = 1) {
			switch (direction.value()) {
			case MoveDirection::Right:
				c += steps; break;
			case MoveDirection::RightUp:
				c += steps; r -= steps; break;
			case MoveDirection::Up:
				r -= steps; break;
			case MoveDirection::LeftUp:
				c -= steps; r -= steps; break;
			case MoveDirection::Left:
				c -= steps; break;
			case MoveDirection::LeftDown:
				c -= steps; r += steps; break;
			case MoveDirection::Down:
				r += steps; break;
			case MoveDirection::RightDown:
				c += steps; r += steps; break;
			}
			return *this;
		}

		// direction の方向に steps 回動いた先の座標を得る
		BoardPos movedAlong(MoveDirection direction, int32 steps = 1) {
			BoardPos res = *this;
			return res.moveAlong(direction, steps);
		}

		friend void Formatter(FormatData& formatData, const BoardPos& value) {
			return Formatter(formatData, value.asPoint());
		}

		// c を x に、 r を y にマッピング
		Point asPoint() const noexcept { return Point(c, r); }
	};


	// PlayerColor を添え字としてアクセスする配列
	template<class T>
	class EachPlayer {
	public:

		EachPlayer() : m_red(), m_blue() {}

		EachPlayer(T red, T blue) : m_red(red), m_blue(blue) {}

		void assign(T red, T blue) {
			m_red = std::move(red);
			m_blue = std::move(blue);
		}

		const T& operator[](PlayerColor at) const {
			return at == PlayerColor::Red ? m_red : m_blue;
		}
		T& operator[](PlayerColor at) {
			return at == PlayerColor::Red ? m_red : m_blue;
		}

	private:
		T m_red;
		T m_blue;
	};

}



template <>
struct SIV3D_HIDDEN fmt::formatter<Procon34::BoardPos, s3d::char32>
{
	std::u32string tag;

	auto parse(basic_format_parse_context<s3d::char32>& ctx)
	{
		return s3d::detail::GetFormatTag(tag, ctx);
	}

	template <class FormatContext>
	auto format(const Procon34::BoardPos& value, FormatContext& ctx)
	{
		return format_to(ctx.out(), U"({}, {})", value.r, value.c);
	}
};
