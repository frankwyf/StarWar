# StarWar

English is the primary documentation entry point.

## Documentation

- English (full guide): [docs/README.en-US.md](docs/README.en-US.md)
- 简体中文（完整指南）: [docs/README.zh-CN.md](docs/README.zh-CN.md)
- 日本語（完全ガイド）: [docs/README.ja-JP.md](docs/README.ja-JP.md)

## Quick Start (Players - Windows)

1. Go to **Releases** on GitHub and download `StarWar-windows-x64.zip`
2. Unzip it and run `StarWar.exe`
3. On start screen, pick difficulty with `1/2/3`
4. Press `Enter`, wait for countdown, then start

## Build from Source (Developers)

1. Open `StarWar.sln`
2. Select `x64` and `Debug` or `Release`
3. Build and run project `main`
4. In-game, press `Enter` to open combat from the start/help screen

## Automatic Build/Release

- GitHub Actions workflow: `.github/workflows/release-windows.yml`
- Every push builds a downloadable artifact (`StarWar-windows-x64`)
- Tags like `v1.0.0` also publish a release zip asset automatically
