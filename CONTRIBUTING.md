# CONTRIBUTING

## 開発方針

- 主実装言語は C++
- シェーダー処理は `.effect` ファイルで管理
- ビルドシステムは CMake
- パッケージ管理は `vcpkg.json`（manifest）
- 静的解析は Clang-Tidy

## セットアップ

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg_root>/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Debug
```

## チェック

- CMake configure/build が通ること
- パラメータ追加時は `src/obs-scene-lighting.cpp` と `data/scene_lighting.effect` を同時更新すること
- 公開・非公開を問わず、追加するファイル/関数/構造体/列挙型には JDoc ライクコメントを付与すること
