[一覧に戻る](../index.md)

# Visualizer モジュール

目的の異なるビジュアライザの共通部分を実装する。

## 使用前に

```c++
#include "module_visualizer/ver_01.hpp"
using namespace Visualizer_01
```

## class MassDrawer

```c++
	class MassDrawer;
```

マスの描画方法を定義する。

- コンストラクタ
    何も与えない。インスタンスを頻繁に作ってもよい。

    ```c++
        MassDrawer();
    ```

- メンバ関数

    - `rect` に正方形を設定し、適切な情報を渡すと、 `rect` の領域内にマスの情報を描画する。

        ```c++
            void drawNormalBiome(RectF rect);
            void drawPondBiome(RectF rect);
            void drawCastleBiome(RectF rect);

            void drawBiome(MassBiome biome, RectF rect);
            void drawArea(PlayerColor color, bool closed, RectF rect);
            void drawWall(PlayerColor color, RectF rect);
            void drawAgent(PlayerColor color, RectF rect);
        ```

## class BoardDrawer

```c++
    class BoardDrawer;
```

（各マスの描画方法に加えて、）盤面を描画する領域と、盤面のマスの個数から、各マスの位置を計算する方法を管理する。

盤面を 1 回描画するたびに、数回構築する程度なら大丈夫。

- コンストラクタ
    - 盤面を描画する領域と、盤面のマスの個数 `(x = row, y = col)`

    ```c++
        BoardDrawer(RectF outerRect, Size boardSize);
    ```

- メンバ関数

    - `pos` にマスの座標を設定し、適切な情報を渡すと、その座標のマスの情報を描画する。

        ```c++
            void drawNormalBiome(BoardPos pos);
            void drawPondBiome(BoardPos pos);
            void drawCastleBiome(BoardPos pos);

            void drawBiome(MassBiome biome, BoardPos pos);
            void drawArea(PlayerColor color, bool closed, BoardPos pos);
            void drawWall(PlayerColor color, BoardPos pos);
            void drawAgent(PlayerColor color, BoardPos pos);
        ```

    - `RectF getMassRect(BoardPos pos)` : 座標 `pos` のマスが描画される領域を得る。
    - `void drawFullBiome(const Grid<MassBiome>& grid)` : すべてのマスの環境を一度に描画する。
