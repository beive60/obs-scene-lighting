# CONTRIBUTING

## 開発方針

- 主実装言語は C++
- シェーダー処理は `.effect` ファイルで管理
- ビルドシステムは CMake
- OBS 依存関係は `buildspec.json` と CMake bootstrap で管理
- 静的解析は Clang-Tidy

## セットアップ

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64
.\scripts\install-portable-obs.ps1
```

- 初回 configure で `.deps` 配下に OBS sources / `obs-deps` を取得し、`libobs` を bootstrap します。
- 公式前提に合わせて Windows は Visual Studio 2022 generator を使います。
- portable OBS に直接配置する場合は `cmake --install build_x64 --config <Config> --prefix <OBS root>` を使ってください。
- `scripts/install-portable-obs.ps1` は configure / build / install を 1 コマンドに束ねた補助 script です。

## チェック

- CMake configure/build が通ること
- `build_x64/rundir/<Config>` に OBS runtime 互換レイアウトが出力されること
- パラメータ追加時は `src/obs-scene-lighting.cpp` と `data/scene_lighting.effect` を同時更新すること
- 公開・非公開を問わず、追加するファイル/関数/構造体/列挙型には JDoc ライクコメントを付与すること
