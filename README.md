# obs-scene-lighting

OBS Studio 32.1.2 (64 bit) / Windows 11 向けの Scene Lighting フィルタープラグインです。  
AviUtl2 スクリプト [aviutl2-scene-lighting](https://github.com/beive60/aviutl2-scene-lighting) のロジックを OBS Source に適用できるようにした実装です。

## 機能

- 前景 Source にフィルターを適用
- 背景 Source をドロップダウンから選択
- AviUtl2 版に準拠した主要パラメータ
  - ブレンドモード（乗算 / スクリーン / オーバーレイ / 加算）
  - 強度
  - ブラー半径
  - エッジ幅 / エッジ閾値
  - ライトラップ / リムライト
  - リムライト角度
  - ベースカラー
  - サンプリング方式（平均 / 中央点）

## 必要環境

- Windows 11
- OBS Studio 32.1.2 (64 bit)
- Visual Studio 2022 / MSVC x64 ビルド環境
- CMake 3.28+
- Clang-Tidy（任意、品質チェック用）
- インターネット接続（初回 configure で依存取得に使用）

このリポジトリは `obs-plugintemplate` に近い Windows 向け bootstrap 構成へ移行しています。初回 configure 時に `.deps` 配下へ OBS sources と `obs-deps` を取得し、そこから `libobs` を最小ビルドします。

## ビルド

```powershell
$cmake = "C:\Program Files\CMake\bin\cmake.exe"

& $cmake --preset windows-x64
& $cmake --build --preset windows-x64
```

PowerShell で `"C:\Program Files\CMake\bin\cmake.exe" -S ...` のように実行すると、先頭の引用付きパスが文字列として解釈されて `Unexpected token '-S'` になります。引用付き実行ファイルパスを使う場合は `&` を付けてください。

初回 configure では次を自動実行します。

- `.deps` へ `OBS-Studio-32.1.2-Sources.tar.gz` を取得して展開
- `.deps` へ `windows-deps-2026-05-21-x64.zip` を取得して展開
- 取得した OBS sources から `libobs` を最小ビルドして `.deps` に install

build 出力の `build_x64/rundir/RelWithDebInfo` には、OBS runtime 互換のレイアウトでプラグインがコピーされます。

提供済みの portable OBS へ直接配置する場合は、install prefix を OBS root に向けて install してください。

```powershell
& $cmake --install build_x64 --config RelWithDebInfo `
  --prefix "C:\Apps\OBS-Studio\OBS-Studio-32.1.2-Windows-x64"
```

configure / build / install を 1 コマンドで流す場合は、補助 script を使えます。

```powershell
& .\scripts\install-portable-obs.ps1
```

既定では `windows-x64` preset、`RelWithDebInfo`、`C:\Apps\OBS-Studio\OBS-Studio-32.1.2-Windows-x64` を使います。

## インストール

`cmake --install` の出力先には OBS runtime の標準配置を使います。

- `obs-scene-lighting.dll` → `obs-plugins/64bit`
- `scene_lighting.effect` / locale → `data/obs-plugins/obs-scene-lighting`

## 使い方

1. 前景 Source に `Scene Lighting` フィルターを追加
2. `背景 Source` で照明情報を取得したい Source を選択
3. 必要に応じて各パラメータを調整

## 開発ドキュメント

- [CONTRIBUTING.md](CONTRIBUTING.md)
- [doc/architecture.md](doc/architecture.md)
