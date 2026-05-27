# Architecture

## 概要

`obs-scene-lighting` は OBS のフィルタープラグインとして動作し、前景 Source に対して背景 Source の環境光情報を合成します。  
処理の中心は `src/obs-scene-lighting.cpp` と `data/scene_lighting.effect` です。

## 構成

- `src/obs-scene-lighting.cpp`
  - OBS プラグイン登録
  - プロパティ定義（背景 Source 選択を含む）
  - パラメータ更新
  - 背景 Source のオフスクリーンレンダリング
  - effect への uniform 設定
- `data/scene_lighting.effect`
  - ブレンドモード
  - エッジ判定
  - ライトラップ
  - リムライト
  - サンプリング方式（平均 / 中央点）

## データフロー

1. フィルター対象 Source に対して `video_render` が呼ばれる
2. 選択された背景 Source を中間テクスチャへレンダリング
3. 前景テクスチャと背景テクスチャを effect に渡す
4. ピクセルシェーダーで環境色抽出とエッジベース合成を行い出力

## パラメータ対応

AviUtl2 スクリプトの主要項目を OBS 側のプロパティとして実装しています。
