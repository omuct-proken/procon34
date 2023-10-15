# game_agent

## struct AgentMarker
```c++
	struct AgentMarker {
		PlayerColor color;
		int32 index;

		bool operator==(const AgentMarker& other) const { return color == other.color && index == other.index; }
		bool operator!=(const AgentMarker& other) const { return !operator==(other); }
	};
```
Agent の色と番号を管理する。
- operator (bool のみ)
    - ==
    色と番号が同じ場合のみ `true` を返し、それ以外の場合 `false` を返す。
    - !=
    色と番号が違う場合に `true` を返す。


## class Agent
```c++
    class Agent {
    public:

        Agent(AgentMarker marker, BoardPos initialPos);

        AgentMarker getMarker() const noexcept;

        BoardPos getPos() const noexcept;

        PlayerColor getColor() const noexcept;

        void setPos(BoardPos pos) noexcept;
    };
```

- コンストラクタ
    - AgentMarker (Agent の色と番号) と BoardPos の初期位置を与えて初期化する

- 関数
    - `getMarker()` : Agent の色と番号を返す。
    - `getPos()` : Agent のグリッド上での位置を返す。
    - `getColor()` : Agent の色（Red, Blue のいずれか）を返す。
    - `setPos(BoardPos pos)` : BoardPos を与えて位置を変更する。

## struct AgentMove

- 関数
    - AgentMove 型
        - `GetNull()` : なにですかこれ
        - `GetStay()` : 

    - 状態の判別
        - `isStay()` : Agent が「滞在」状態であるかを返す
        - `isMove()` : Agent が「移動」状態であるかを返す
        - `isConstruct()` : Agent が「建築」状態であるかを返す
        - `isDestroy()` : Agent が「解体」状態であるかを返す
    - 詳しい状態を取得する関数
        - `asStay()` : agent のポインタを返す
        - `asMove()` : agent のポインタと向き（MoveDirection 型）を返す
        - `asConstruct()` : agent のポインタと向き（MoveDirection 型）を返す
        - `asDestroy()` : `agent のポインタと向き（MoveDirection 型）を返す
        - これらの関数に対応する `is**` が `true` でないような場合には直ちに例外を投げる。
