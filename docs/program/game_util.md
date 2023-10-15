
# game_util

## enum class PlayerColor

2 人のプレイヤーを区別するために使用する。

```c++
	enum class PlayerColor {
		Red,
		Blue
	};
```



## enum class MassBiome

初期状態における領域の分類を表す。この分類がゲーム中に変化することはない。

- `Pond` : 池。
- `Castle` : 城。
- `Normal` : 平地。

```c++
	enum class MassBiome {
		Pond,
		Castle,
		Normal
	};
```


## class MoveDirection

8 方位を表す 0 以上 8 未満の整数を管理する。

右方向が 0　、反時計回りに 45 度回るごとに +1 する。

- コンストラクタ

    - 範囲外の整数を与えると例外を投げる。

- 関数
    - `ccw45Deg(x)` : 反時計回りに 45 * x 度回転した向きを取得する。
    - `is4Direction()` : 4 方位（斜めではないということ）であるときに true 、そうでなければ false を返す。
    - `value()` : 管理している整数を返す。

Formatter 定義済み

## class BoardPos

盤面上の位置を表す。

- 関数

	- `BoardPos& moveAlong(MoveDirection direction, int32 steps = 1)` :
	    `direction` の方向に `steps` マスだけ移動する。

	- `BoardPos movedAlong(MoveDirection direction, int32 steps = 1)` :
	    `direction` の方向に `steps` マスだけ移動した後の位置を返す。

	- asPoint()

Formatter 定義済み 、 `fmt::formatter` 定義済み


## class EachPlayer\<T\>

各プレイヤーに対してデータを持ちたいときに使う。

`T` 型のデータをちょうど 2 つ管理し、 PlayerColor を添え字にしてアクセスできる。

- コンストラクタ

    `red` , `blue` それぞれのデータを与えて初期化する。

	`T` デフォルトコンストラクタがあれば、 `EachPlayer<T>` でもデフォルトコンストラクタが使える。

- 関数

    - `assign(T red, T blue)` : 再構築。

    - `operator[]` : 添え字は `PlayerColor` 型。



