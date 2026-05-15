# StarWar (English)

A small open-source 2D space shooter built with modern C++ and OpenGL/GLFW.

## What You Can Do In-Game

- Pilot your ship with mouse aiming + thrust controls
- Choose difficulty before each run (`Easy / Normal / Hard`)
- Start each run with a short countdown instead of instant danger
- Fight moving asteroids, including split asteroids
- Battle enemy fighters that track and shoot at you
- Defeat a boss every 4th wave
- Progress through waves and weapon upgrades
- Manage lives, shield, invulnerability windows, and positioning

## Gameplay Rules (How to Have Fun)

### Objective

Survive as long as possible, clear waves, defeat bosses, and maximize score.

### Core Loop

1. Start from the help screen (`Enter`)
2. Dodge hazards while shooting asteroids and enemies
3. Clear enemies to advance wave
4. Defeat boss on waves `4, 8, 12, ...`
5. Repeat with harder pressure and higher score opportunities

### Controls

- **1 / 2 / 3**: Select difficulty on start screen
- **Enter**: Start game from help screen (with countdown)
- **Mouse**: Aim ship direction
- **Left Mouse**: Fire
- **W** or **Up Arrow**: Thrust
- **Right Mouse**: Thrust (alternative)
- **Space**: Toggle pilot mode
- **R**: Restart run
- **Esc**: Quit

### Combat and Progression

- Asteroids can be destroyed for score
- Large asteroid hits can create smaller split asteroids
- Enemy ships fire toward the player with slight spread (not perfect hitscan)
- Bosses have much higher HP and multi-shot attacks
- Weapon levels improve as waves increase:
  - **Lv1**: Single shot
  - **Lv2**: Tri-shot spread
  - **Lv3**: Faster fire + stronger damage

### Survival Tips

- Keep moving: stationary play gets overwhelmed quickly
- Use short thrust bursts to dodge enemy bullets
- Strafe while aiming; your bullets and enemies are world-relative
- During invulnerability, reposition aggressively

## HUD / Feedback

- Window title shows **Score / Lives / Wave / Weapon level / Difficulty**
- Top-left: score progress bar
- Top-left (second bar): shield bar
- Top-right: life indicators
- Bosses display a health bar over their ship

## Play Without Building (Recommended)

1. Open the repository **Releases** page
2. Download `StarWar-windows-x64.zip`
3. Extract and run `StarWar.exe`

## Build from Source

### Windows (Visual Studio)

1. Open `StarWar.sln`
2. Select `x64` and `Debug` or `Release`
3. Build and run project `main`

### Cross-platform (Premake + Make)

```bash
premake5 gmake2
make config=release_x64
```

## Automated Build and Release

- Workflow file: `.github/workflows/release-windows.yml`
- Every push to `main/master` builds a downloadable artifact
- Tags like `v1.0.0` publish `StarWar-windows-x64.zip` to GitHub Releases

## Project Structure

- `main/` – game executable and gameplay loop
- `draw2d/` – software rasterization and drawing primitives
- `support/` – runtime/configuration helpers
- `vmlib/` – math primitives (`Vec2f`, `Mat22f`)
- `lines-test/`, `triangles-test/` – unit tests
- `blit-benchmark/`, `lines-benchmark/` – benchmark targets
- `third_party/` – vendored dependencies

## Community & release files

- Contribution guide: `CONTRIBUTING.md`
- Code of conduct: `CODE_OF_CONDUCT.md`
- Security policy: `SECURITY.md`

## Third-party licenses

See `third_party.md` and files under `third_party/`.

## License

This project is licensed under the MIT License. See `LICENSE`.
