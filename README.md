# SNAP

C++ のトレーニングの一環として開発したCLIパーサ

既存の CLI パーサを利用するのではなく，自分で実装することで C++ の理解を深めることが目的．そのため，本プロジェクトは実用的な CLI パーサを新たに提供することを主目的としたものではない．**要は「車輪の再発明」**．

---

## 開発の経緯

C++ の学習を進める中で，CLI アプリケーションを作る機会が増え，CLI パーサが必要になりました．

既存のライブラリを利用することもできますが，CLI パーサは C++ の様々な機能を実践的に学ぶ題材として適していると考えました．

そこで，

「必要になったのなら，自分で作ればトレーニングにもなるのでは」

という考えから，本プロジェクトの開発を始めました．

---

## 特徴
- CLI オプションおよび positional argument の解析
- 型に応じた値のパース
- デフォルト値の設定
- short option / long option
- Built-in command のサポート
- C++23 の機能を活用した実装

---

## 必要環境
- C++23 に対応したコンパイラ
- C++23 に対応した標準ライブラリ

本プロジェクトでは C++23 の機能を使用しているため，ビルドには C++23 をサポートするコンパイラが必要です．

---

## ビルド
``` bash
git clone https://github.com/Dize-Azki6674/SNAP.git
cd SNAP

cmake -S . -B build
cmake --build build
```

---

## 使用方法

SNAP では，`App` と `Arg` を用いて CLI の引数を定義します．

### 基本的な例

``` cpp
#include <snap/snap.hpp>

int main(int argc, char* argv[])
{
    snap::App app("myapp");

    app.arg(
        snap::Arg<int>("number")
            .shorter('n')
            .longer("number")
    );

    app.parse(argc, argv);

    const int number = app.get<int>("number");

    // ...
}
```

CLI では次のように使用できます．

``` bash
$ myapp --number 42
```

### 位置引数

`shorter()`や`longer()`を指定しない`Arg`は*位置引数(positional argument)*として扱われます．

``` cpp
app.arg(
    snap::Arg<std::string>("input")
);
```

``` bash
$ myapp example.txt
```

### デフォルト値

デフォルト値を設定できます．

``` cpp
app.arg(
    snap::Arg<int>("number")
        .longer("number")
        .def(10)  // --number が指定されない場合，値は10になる
);
```

### 複数値を受け取る引数

1つの引数に複数の値を指定することもできます．

``` cpp
app.arg(
    snap::Arg<int, 3>("numbers")
        .longer("numbers")
        .entry("num1")
        .entry("num2")
        .entry("num3")
);
```

``` bash
$ myapp --numbers 1 2 3
```

### オプション
短いオプション，長いオプションはそれぞれ`shorter()`と`longer()`で設定できます．

``` cpp
app.arg(
    snap::Arg<std::string>("output")
        .shorter('o')
        .longer("output")
);
```

``` bash
$ myapp -o result.txt
```

または

``` bash
$ myapp --output result.txt
```

単一のbool値を受け取るオプションでは短縮オプションの連結が許可されます．

### 組み込みコマンド

SNAPには，HelpやVersionなどの*組み込みコマンド*が用意されています．
デフォルトではこれらが有効になっているため，アプリケーション側で明示的に実装する必要はありません．

``` bash
$ myapp --help
$ myapp --version
```

組み込みコマンドは設定によって無効化できます．

``` cpp
app
    .builtins({
        .help{false}
        .version{false}
    });
```

### パース結果の取得
パースした値は`App`から取得できます．
``` cpp
auto result = app.parse(argc, argv);

const auto number = result.view<int>().begin()[0];
```

SNAPは型情報を利用して値をパースするため，整数や浮動小数点数，文字列などをそれぞれ対応する型として取得できます．

> APIは開発中であり，今後変更される可能性があります．

---

## 位置づけ

SNAP は個人の C++ トレーニングを目的としたプロジェクトです．

既存の成熟した CLI パーサには，より多くの機能や長期的な保守性が期待できます．そのため，実用上は既存のライブラリを利用する方が適切な場合があります．

本プロジェクトでは，「既存のものを使う」ことよりも「自分で作って理解する」ことを重視しています．

---

## 関連プロジェクト

本プロジェクトは C++ 学習プロジェクト群の一環として開発されています．

他のトレーニングプロジェクトについては以下を参照してください．

| プロジェクト | 状況 |	リンク                                  | 概要                    |
|--------------|------|-----------------------------------------|-------------------------|
| mywc         | 完了 |	https://github.com/Dize-Azki6674/mywc   | wc 風行数カウントツール |
| mygrep       | 完了 | https://github.com/Dize-Azki6674/mygrep | grep 風文字列検索ツール |

---

## 作者

Azkey