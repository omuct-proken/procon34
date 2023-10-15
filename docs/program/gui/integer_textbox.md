[GUI素材一覧に戻る](index.md)

# IntegerTextBox

```c++
class IntegerTextBox
```

設定された範囲の整数が十進法で入力された場合のみ有効値とし、有効な場合は、入力された整数を取得できる。

無効な場合は、描画するときに特定の効果をつけたうえで、無効である理由を表すエラーメッセージを取得できる。

## コンストラクタ

```c++
IntegerTextBox(
    StringView name = U"noname",
    int64 limit_min = INT64_MIN,
    int64 limit_max = INT64_MAX,
    Optional<int64> val_default = none
)
```

- `name` : テキストボックスの名前。エラーメッセージに使用される。
- `limit_min` : 許容する値の下限。
- `limit_max` : 許容する値の上限。
- `val_default` : 規定値。 none では空になる。

## メンバ関数

- `limit`

  ```c++
  void limit(int64 lim_min, int64 lim_max);
  ```

  許容する値の範囲を変更する。

- `update`

  ```c++
  void update(RectF rect, double padding = 3.0);
  ```

  状態の更新と、描画を行う。領域 rect の左上に描画する。 `padding` は無効値のときの効果のためのスペースの幅。

- `isOk`

  値が有効なら true

- `value`

  値が有効なら、その値を返す。それ以外の場合の動作は規定しない。

- `getErrorMessage()`

  エラーメッセージを返す。
