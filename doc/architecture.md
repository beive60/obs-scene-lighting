# Architecture

## 概要

`obs-scene-lighting` は OBS のフィルタープラグインとして動作し、前景 Source に対して背景 Source の環境光情報を合成します。  
処理の中心は `src/obs-scene-lighting.cpp` と `data/scene_lighting.effect` です。

Source filter は通常 Source ローカル座標で描画されますが、本実装では前景 Source をアクティブな scene graph 上の `obs_sceneitem_t` に解決できた場合、scene-space の変換を使って背景サンプリング位置を補正します。これにより、前景の平行移動、拡大縮小、回転に追従した背景サンプリングを行います。

## 構成

- `src/obs-scene-lighting.cpp`
  - OBS プラグイン登録
  - プロパティ定義（背景 Source 選択を含む）
  - パラメータ更新
  - 前景 Source の scene item 探索
  - scene-space UV マッピング生成
  - 背景 Source のオフスクリーンレンダリング
  - effect への uniform 設定
- `data/scene_lighting.effect`
  - ブレンドモード
  - エッジ判定
  - ライトラップ
  - リムライト
  - サンプリング方式（平均 / 中央点）
  - scene-space UV に基づく背景サンプリング

## Scene 解決

- `video_render` 時に、フィルター対象の前景 Source を program scene、preview scene、その他の scene 一覧から探索します。
- 探索時は group と nested scene を再帰的にたどり、`obs_sceneitem_get_draw_transform` で得た変換を累積します。
- 一意に scene item を特定できた場合だけ、その draw transform から前景 UV を scene-space UV に変換するための origin / X 軸 / Y 軸ベクトルを生成します。
- 同じ Source が複数の scene item として見つかった場合は曖昧とみなし、scene-space マッピングは無効化します。

## データフロー

1. フィルター対象 Source に対して `video_render` が呼ばれる
2. 前景 Source を scene graph 上の `obs_sceneitem_t` に解決し、成功時は scene-space UV マッピングを生成する
3. 選択された背景 Source を中間テクスチャへレンダリングする
4. 前景テクスチャ、背景テクスチャ、texel size、scene-space UV マッピングを effect に渡す
5. ピクセルシェーダーで環境色抽出とエッジベース合成を行い出力する

scene-space マッピングが有効な場合、背景は Source サイズではなく root scene のキャンバスサイズでレンダリングされます。これにより、前景の変形後の scene 上の位置に対応する背景領域を参照できます。

scene-space マッピングが無効な場合は、shader 側で従来どおり前景 UV をそのまま背景 UV として扱います。

## パラメータ対応

AviUtl2 スクリプトの主要項目を OBS 側のプロパティとして実装しています。
