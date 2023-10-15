# game_board

## struct WallData
```c++
    struct WallData {
        PlayerColor color;
    };
```
陣地の色を管理する。

## struct Mass

マスの情報（平地 or 城 or 池 であるかに加えて、Agent が存在するか, 壁が建築されているか）を管理する。

- 関数
    - `hasWall()` : 壁がマスに建築されている（正確には、有効な値が wall に保存されているか）ならば `true`, ないなら `false` を返す
    - `hasAgent()` : Agent がマスにいる（正確には、有効な値が agent に保存されているか）ならば `true`, ないなら `false` を返す
    - `hasWallOf(PlayerColor color)` : 壁がマスに建築されていて、なおかつ color と同じ色の壁ならば `true`, 違う色ならば `false` を返す。

## class GameBoard

- コンストラクタ
    - 盤面の 横幅 (1 <= height <= 500)と 縦幅 (1 <= width <= 500) を与えて初期化する。
- 関数
    - bool 型
        - `isOnBoard()` : 座標（BoardPos 型）を与えて、それが盤面に載っているならば `true`、範囲外ならば `false` を返す。
    - 盤面の状態を取得する
        - `getWidth()` : 盤面の 横幅 を取得する
        - `getHeight()` : 盤面の 縦幅 を取得する
        - `getSize()` : 盤面の大きさ（横幅, 縦幅のペア）を取得する
        - `getMass()` : 座標 `pos` のマスの状態を返す。そのマスが存在しなければ例外を投げる。
        - `getMassOr()` : 座標 `pos` を与えて、それが盤面に載っているならば(`isOnBoard(pos) == true`)その座標のマスの状態を返し、載っていないならば `ex` を返す
- operator
    - `[]` : 座標を map っぽく投げると、その座標のマスの状態が返ってくる。座標が範囲外ならば out of index みたいな error が出る
