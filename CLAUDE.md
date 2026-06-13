# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Plants vs. Zombies clone implemented in C++17 using the **PTSD framework** (Practical Tools for Simple Design v0.2). 2D tower-defense game with 10 levels, multiple plant and zombie types, and a wave-based spawning system.

## Build Commands

```bash
# Configure (Debug mode required — Release build path is broken)
cmake -DCMAKE_BUILD_TYPE=Debug -B build

# Build
cmake --build build

# Run
./build/PvZ
```

> **Important:** Always build in Debug mode. The Release configuration has a broken resource path and will not find assets at runtime.

## Architecture

### Entry Point & Game Loop

`src/main.cpp` runs a tight loop:
1. Check exit flag
2. Call `App::Setup()` once
3. Dispatch to `App::Update()` (or sub-state variants) by `App::State`
4. Render ImGui overlays
5. Tick PTSD context

`App` is a monolithic game controller (`include/App.hpp` / `src/App.cpp`, ~3000 lines) that owns all game state and drives every system each frame.

### State Machine

`App::State` drives the top-level flow:
```
START → MENU → GAME_LOADING → PLAYING → LEVEL_COMPLETE / LEVEL_FAILED → END
                                  ↕
                               PAUSED
```

`PLAYING` ↔ `PAUSED` via the pause button; `PAUSED` can also return to `MENU`.

Inside `PLAYING`, a camera intro sequence (3 stages: HOME → RIGHT → CENTER) must complete before sun drops, wave spawning, and the card UI become active.

### Grid System

The game board is a **5 row × 9 column** grid mapped to screen percentages:
- X: 5%–95% of 1280px
- Y: 10%–90% of 720px

`App` maintains flat arrays/vectors indexed by `row * 9 + col`. Key grid helpers in `App.cpp`:
- `ComputeGridCellLocalPosition(row, col)` — grid → screen position
- `HandleGridClick(xPct, yPct, ...)` — mouse → grid cell
- `IsCellOccupied(index)`
- `PlaceXxxAtGridCell(row, col)` — one method per plant type

### Entity Hierarchy

```
Util::GameObject (PTSD)
├── Plant (include/Plant.hpp)
│   ├── Sunflower, Sunshroom
│   ├── Peashooter, Puffshroom, Fumeshroom
│   ├── Nut
│   └── CherryBomb
└── Zombie (include/Zombie.hpp)
    ├── BasicZombie, ConeheadZombie, LeaderZombie
    └── PolevaultingZombie
```

Plants and zombies do **not** update themselves — `App::UpdateGameplay()` and the various `App::UpdateXxx()` methods drive all logic centrally.

### Collision System

All AABB detection is centralized in `CollisionSystem` namespace (`include/CollisionSystem.hpp`, `src/CollisionSystem.cpp`). Use the predefined `CollisionBoxType` enum for standard entities, or `CheckCustomAABBCollision()` for non-standard hitboxes. CherryBomb uses grid-distance collision (`CheckCherryBombExplosionCollision`), not pixel AABB. See `COLLISION_SYSTEM.md` for hitbox specs per entity.

### Level & Wave Configuration

Levels are JSON files in `Resources/levels/level{1-10}.json`. Loaded by `LevelConfigLoader::LoadFromFile()` into `LevelConfig` + `ZombieWavePhaseConfig` structs.

Wave phase types: `setup`, `sub`, `huge`, `pause`. Each phase defines zombie types, spawn intervals, repeat count, and a `waitUntilClear` flag.

`LevelConfig` also specifies `allowedPlants` — the card bar only shows plants listed for that level.

### Extracted Systems

Two subsystems have been refactored out of `App` into their own classes:

| Class | File | Responsibility |
|---|---|---|
| `SunManager` | `include/SunManager.hpp` / `src/SunManager.cpp` | Owns all `Sun` objects; drives sky-sun spawning, plant-sun production, collection animation. `App` holds the sunlight count. |
| `ZombieWaveController` | `include/ZombieWaveController.hpp` / `src/ZombieWaveController.cpp` | Builds and advances the wave spawn plan from `LevelWaveConfig`. |

`App` holds a `SunManager m_SunManager` and a `ZombieWaveController m_WaveController` as value members. Both are reset via `Reset()` inside `ResetLevelRuntimeState()`.

### Key Methods in App.cpp

| Method | Responsibility |
|---|---|
| `UpdateCamera()` | 3-stage cinematic pan at level start |
| `UpdateBasicZombie()` | Consults `ZombieWaveController`, spawns zombie objects |
| `UpdatePeashooterCombat()` | Pea projectile spawning and movement |
| `UpdateCherryBombs()` | Detonation timer + explosion damage |
| `UpdateLawnMowers()` | Activation on zombie contact, row sweep |
| `HandleGridClick()` | Plant placement / shovel removal |
| `InitializeLevel()` | Calls `ResetLevelRuntimeState()` then rebuilds scene graph |
| `ResetLevelRuntimeState()` | Clears all per-level state (plant arrays, bullets, zombies, renderers) |

### Asset Pipeline

GIF sprites are extracted to frame PNGs at startup (`App::Start()`). Extracted frames land in `Resources/.../frames/` subdirectories. Adding a new animated entity requires pre-extracting its GIF or adding it to the startup extraction list.

### Debug Mode

`src/AppDebug.cpp` provides ImGui overlays with collision box visualization and cheat toggles (unlimited plants, no cooldowns). Activated via a flag in `App`.

## Dependencies

Fetched automatically by CMake via `FetchContent`:
- **PTSD v0.2** — framework (GameObject, Animation, Renderer, Input, SDL2, OpenGL)
- **nlohmann_json v3.11.3** — level JSON parsing

Custom PTSD overrides live in `ptsd_overrides/` (Renderer, Image, Shaders). CMake uses `configure_file` to copy them into the PTSD source tree at configure time, replacing the originals before compilation. Three overrides:
- `Util::Renderer` — adds `SetTranslation()` for camera-pan offset
- `Util::Image` — adds `SetTintColor()` / `SetFillProgress()` for card cooldown UI
- `Base.frag` — GLSL implementation of tint and fill-progress effects

## Adding New Content

**New plant:** Subclass `Plant`, implement constructor with sprite/animation paths, add a `std::array<std::shared_ptr<XxxPlant>, kGridCellCount>` in `App.hpp`, add a `PlaceXxxAtGridCell()` method, wire into `UpdateGameplay()` and the card-slot UI, register in the level JSON `allowedPlants` array. Also add `.fill(nullptr)` for the new array in `ResetLevelRuntimeState()` and include it in `IsCellOccupied()` — omitting either causes stale references or grid bugs across level resets.

**New zombie:** Subclass `Zombie`, set stats in constructor (speed, health, damage), add spawn logic in `UpdateBasicZombie()` or a dedicated `UpdateXxxZombie()`, add a `CollisionBoxType` entry if hitbox differs from the default.

**New level:** Copy an existing `Resources/levels/levelN.json`, increment `id`, adjust `allowedPlants`, `initialSun`, `sceneType`, and `waves` phases.
