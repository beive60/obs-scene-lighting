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
- CMake 3.24+
- vcpkg（manifest mode）
- Clang-Tidy（任意、品質チェック用）
- libobs 開発環境

## ビルド

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg_root>/scripts/buildsystems/vcpkg.cmake
cmake --build build --config RelWithDebInfo
```

## インストール

ビルド成果物を OBS のプラグイン配置先に配置してください。

- `obs-scene-lighting.dll` → `obs-plugins/64bit`
- `scene_lighting.effect` → `data/obs-plugins/obs-scene-lighting`

## 使い方

1. 前景 Source に `Scene Lighting` フィルターを追加
2. `背景 Source` で照明情報を取得したい Source を選択
3. 必要に応じて各パラメータを調整

## 開発ドキュメント

- [CONTRIBUTING.md](CONTRIBUTING.md)
- [doc/architecture.md](doc/architecture.md)
