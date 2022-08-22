# 秋霜玉

## Building

Currently, this project can only be built for 32-bit Windows, with Visual Studio ≥2022 and a Tup-based build setup.
However, since IDE integration is horribly broken for both Makefile and directory projects, we strongly recommend literally *anything else* to edit the code.

You'll therefore need the following:

1. Visual Studio Community ≥2022, with the *Desktop development for C++* workload.\
   If you haven't already installed the IDE for other projects and don't plan to, you can install only the command-line compilers via the [Build Tools installer](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022).

2. [Tup](https://gittup.org/tup/) in the latest Windows version.\
   Place `tup.exe` and its DLLs somewhere in your `PATH`.

3. Your favorite code editor.

To build:

1. Open Visual Studio's *x64_x86 Cross Tools Command Prompt*.
2. Navigate to the checkout directory of this repository.
3. Invoke `tup` in your way of choice.

The binary will be put into the `bin/` subdirectory, where you can also place the game's original data files.

By default, both Debug and Release configurations are built.
If you only need one of them and want to speed up the build process, you can deactivate either one by creating a file named `tup.config` in the root directory of this repository:

```sh
CONFIG_DEBUG=n   # deactivates Debug mode if present
CONFIG_RELEASE=n # deactivates Release mode if present
```

## Debugging

.PDB files are generated for Debug and Release builds, so you should get symbol support with any Windows debugger.

### Visual Studio IDE

We don't support it for compilation, but you can still use it for debugging by running

```bat
devenv bin/GIAN07d.exe &::to run the Debug binary
devenv bin/GIAN07.exe  &::to run the Release binary
```

from the *x64_x86 Cross Tools Command Prompt*.
Strangely enough, this yields a superior IntelliSense performance than creating any sort of project. 🤷

----

Original README by pbg below.

----

## これは何？
* 西方プロジェクト第一弾 **秋霜玉** のソースコードです。
* コンパイルできるかもしれませんが, すべてのソースコードが含まれているわけではないのでリンクはできません。
* 画像、音楽、効果音、スクリプト等のリソースは含まれません。


## 参考までに
* 基本、開発当時（2000年前後）のままですが、文字コードを utf-8 に変更し、一部コメント（黒歴史ポエム）は削除してあります。インデント等も当時のままなので、読みにくい箇所があるかもしれません。
* 8bit/16bitカラーの混在、MIDI再生関連、浮動小数点数演算を避ける、あたりが懐かしポイントになるかと思います。
* 8.3形式のファイル名が多いのは、PC-98 時代に書いたコードの一部を流用していたためです。
* リソースのアーカイブ展開に関するコードはもろもろの影響を考え、このリポジトリには含めていません。


## ディレクトリ構成
* /**MAIN** : 秋霜玉WinMainあたり
* /**GIAN07** : 秋霜玉本体
* /**DirectXUTYs** : DirectX, MIDI再生、数学関数等の共通処理
* /**MapEdit2** : マップエディタ
* /**ECLC** : ECL(敵制御用) スクリプトコンパイラ
* /**SCLC** : SCL(敵配置用) スクリプトコンパイラ


## たぶん紛失してしまったソースコード
以下のコードについては、見つかり次第追加するかもしれません。
* リソースのアーカイバ


## ライセンス
* [MIT](LICENSE)
