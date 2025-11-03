# Celestium — developer notes for AI assistants

## Project overview
Celestium is a 2D space-station simulation written in modern C++ (C++23) using raylib for rendering and input. Primary systems include:

- Procedural station/tile system with data-driven tile definitions
- Crew AI and pathfinding (A*), actions and queued behaviours
- Environmental simulation (oxygen diffusion, environmental effects)
- Power grid simulation with prioritized consumers and batteries
- Particle systems with small Lua scripts for behaviour hooks

Definitions and tuning are stored as YAML under `assets/definitions/`.

## Core architecture

### Component system
Tiles are assembled from small Components that encapsulate behaviour. Common components:

- WalkableComponent — allows crew movement
- SolidComponent — blocks vision/pathing
- PowerConnectorComponent — connects to power grids
- BatteryComponent — stores/supplies power
- PowerProducer / PowerConsumer — producers/consumers and optional priority
- OxygenComponent / OxygenProducer — oxygen storage and generation
- DoorComponent — animated door which affects air flow
- DurabilityComponent — hitpoints and damage handling

Usage example:
```cpp
if (auto oxygenComp = tile->GetComponent<OxygenComponent>()) {
    float level = oxygenComp->GetOxygenLevel();
}
```

### Game state and update model

- `GameManager` is the central singleton used to access global state (camera, station, crew lists, Lua state, etc.) and manage game-state transitions (NONE → MAIN_MENU → GAME_SIM).
- A dedicated fixed-update thread (see `src/fixed_update.cpp`) steps the simulation at `FIXED_DELTA_TIME`. It runs crew action handling, environmental updates, power grid updates and prepares a `RenderSnapshot` for the main/render thread.

### Data flow

1. `DefinitionManager` parses `constants.yml`, `tiles.yml` and `env_effects.yml` (in `assets/definitions/`) to populate runtime constants and tile/effect definitions.
2. Tiles are spawned with component lists and sprite conditions from the parsed definitions.
3. The fixed-update loop performs simulation work and publishes a `RenderSnapshot`.
4. The main thread (raylib loop in `src/main.cpp`) handles input, UI, drawing and audio.

## Development workflow

### Notable directories

- `src/` — C++ source and header files
- `assets/definitions/` — YAML config and definitions (`constants.yml`, `tiles.yml`, `env_effects.yml`)
- `assets/tilesets/`, `assets/audio/`, `fonts/` — media assets
- `CMakeLists.txt` — build definitions (external deps are under `external/`)
- `shell.nix` — reproducible development environment

### Build & run

Build with CMake (out-of-source):

    mkdir -p build && cd build
    cmake ..
    make -j$(nproc)

Run the produced `celestium` binary in `build/`.

Use `shell.nix` if you want a nix-based development environment.

### Ownership & memory

- `std::shared_ptr` is used for owning references to tiles, components, crew and definitions.
- `std::weak_ptr` is used for back-references and to break ownership cycles.
- `std::enable_shared_from_this` is used where objects need to safely obtain a shared pointer to themselves.

### Enums & constants

- `magic_enum` is used for enum parsing and name conversion.
- `src/const.hpp` declares global constants and the `FIXED_DELTA_TIME` variable, which is populated at startup by `DefinitionManager::ParseConstantsFromFile()`.

### Pathfinding & actions

- A* pathfinding is implemented in `src/astar.cpp`/`src/astar.hpp`.
- Crew actions are modelled as `Action` subclasses (see `src/action.hpp`/`src/action.cpp`) and progressed during fixed updates. Common actions: MOVE, EXTINGUISH, REPAIR.

### Rendering & UI

- Rendering and input are handled on the main thread using raylib. UI code lives in `src/ui.cpp` and `src/ui_manager.cpp`.
- Tile rendering supports neighbour-aware sprite selection (see `src/tile_def.hpp` and `src/tile.cpp`).

### Logging & errors

- Use `LogMessage()` for structured logging across INFO/WARNING/ERROR levels.
- `DefinitionManager` throws on missing required configuration keys — treat these as hard errors that should be fixed in YAML.

## Dependencies

- raylib — rendering, input and basic audio
- ryml (rapidyaml) — YAML parsing (`src/def_manager.hpp` uses `ryml.hpp`)
- sol2 + LuaJIT — embedded Lua scripting used for particle system hooks
- magic_enum — enum parsing helpers
- RtAudio/Opus — audio utilities

Most dependencies are configured through CMake and/or vendored under `external/`.

## Common tasks & how-to

### Add or change a tile definition

1. Edit `assets/definitions/tiles.yml` and add/modify a `tile` entry.
2. Use the `components` list for components and include component-specific parameters.
3. If a new component type is required, implement it in `src/` and add its construction to `DefinitionManager::CreateComponent()`.

### Add a crew action

1. Create an `Action` subclass and implement its `Update()` logic.
2. Add the action type to the `Action::Type` definition and wire up creation/assignment logic in `AssignCrewActions()` / `HandleCrewActions()`.

### Adjust simulation timing

- Change `general/fixedDeltaTime` in `assets/definitions/constants.yml`; `DefinitionManager::ParseConstantsFromFile()` will populate the `FIXED_DELTA_TIME` global at startup.
- Use `FIXED_DELTA_TIME` for per-second rates to keep behaviour consistent across platforms.

## Quick file map

- `src/main.cpp` — program entry, main render loop and input handling
- `src/fixed_update.cpp` — fixed-step simulation thread
- `src/def_manager.hpp` — YAML parsing, constants and definition creation
- `src/astar.*` — pathfinding
- `src/tile*`, `src/component*` — tile and component logic
- `src/action.*`, `src/crew.*` — actions and crew AI
- `src/ui.*` — UI and overlays
