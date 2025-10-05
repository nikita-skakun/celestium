# Celestium — Agent Task List

Purpose
- A short, curated, and actionable list of tasks for automated coding agents and contributors.
- Each task contains a clear goal, files to inspect/edit, step-by-step suggestions, acceptance criteria, and a suggested difficulty/estimate.

How to use
- Pick an unclaimed task and create a PR branch named `agent/<task-id>-short-desc`.
- Add a short PR description that references this task ID.
- Tests (if applicable) should be added under `tests/` and wired into CMake where feasible.

Task template (use when adding new tasks)
- id: TASK-XXX
- title: Short descriptive title
- priority: P0/P1/P2
- difficulty: (Easy/Medium/Hard)
- files: [list files to read/edit]
- description: What to change and why
- steps: Ordered actionable steps
- acceptance: Concrete checks that prove the task is done

---

Task list (starter)

- id: TASK-001
  title: Rework power wiring to be physical game objects
  priority: P0
  difficulty: Hard (2-4 days)
  files:
    - `src/power_grid.hpp`
    - `src/station.hpp` / `src/station.cpp` (Station::AddPowerWire / RemovePowerWire)
    - `src/component.hpp` (PowerConnectorComponent / BatteryComponent)
    - `src/update.cpp` / `src/fixed_update.cpp` (UpdatePowerGrids)
    - `assets/definitions/tiles.yml` (if new wire tile needed)
  description:
    The project README and TODOs mention that power connections should be part of the physical world. The repository currently stores wire positions inside `PowerGrid::powerWires` (a positional set) and `Station::AddPowerWire`/`RemovePowerWire` handle merge/split and connector attachment. To make wires editable and visible while preserving current behaviour, perform a staged migration:

    - Stage 1 (non-destructive): Add a Station-level infrastructure map and APIs that forward power-wire operations into the existing `PowerGrid` logic. This makes infra visible and editable without breaking grids.
    - Stage 2 (optional refactor): When UI/tests are stable, migrate `PowerGrid` to read from Station's infrastructure map (or be constructed from it), then remove `PowerGrid::powerWires`.
  steps:
    1. Inspect `PowerGrid` in `src/power_grid.hpp` and verify merge/split behaviour used by `Station::AddPowerWire`/`RemovePowerWire`.
    2. Implement Stage 1:
       - Add `enum class InfrastructureType { NONE, POWER_WIRE, ... }` and `std::unordered_map<Vector2Int, InfrastructureType> infrastructureMap;` to `Station`.
       - Add `Station::AddInfrastructure(Vector2Int,pos, InfrastructureType)` and `RemoveInfrastructure(...)` that update `infrastructureMap` and call existing `AddPowerWire`/`RemovePowerWire` for `POWER_WIRE`.
       - Add a simple debug overlay draw to render infrastructure positions (tiny sprite / colored square) so wires are visible in build mode.
    3. Wire build-mode UI to call `AddInfrastructure`/`RemoveInfrastructure` (toggle on click).
    4. Add smoke tests verifying parity with existing behaviour (grid creation, merge, split, connector attachment).
    5. (Optional) Stage 2: Once UI/tests are stable, implement `Station::RebuildPowerGridsFromInfrastructure()` and refactor `PowerGrid` to be constructed from or query Station data; then remove `PowerGrid::powerWires`.
  acceptance:
    - Player can place/remove wires in build mode and see wires on screen (debug overlay OK initially).
    - Behaviour parity: grid creation/merge/split and connector attachment remain unchanged when using the new Station APIs.
    - Tests/smoke checks prove parity; only remove `PowerGrid::powerWires` after tests pass and parity is confirmed.

- id: TASK-002
  title: Add save/load (serialization) for station state
  priority: P0
  difficulty: Medium (1-2 days)
  files:
    - `src/station.hpp` / `src/station.cpp`
    - `src/tile.hpp` / `src/tile.cpp`
    - `src/game_state.hpp` / `src/game_state.cpp`
    - `assets/definitions/*.yml`
  description:
    Persist the current station, tiles, crew, and effect state into a file (YAML or JSON). There is already a data-driven tileset; leverage the same format for saving state.
  steps:
    1. Define a lightweight save format (YAML recommended to stay consistent with existing defs).
    2. Add `Serialize()`/`Deserialize()` methods to `Tile`, `Station` and `Crew`.
    3. Hook save/load into UI main menu (add simple keybind or menu option) or create `tools/save_tool.cpp` for manual testing.
    4. Add a small sample save file under `assets/saves/`.
  acceptance:
    - Loading a save reconstructs station tile layout, components, crew positions, and effect states closely matching before-save state.

- id: TASK-003
  title: Make definitions (tiles & effects) reloadable at runtime
  priority: P1
  difficulty: Medium
  files:
    - `src/def_manager.hpp` / `src/def_manager.cpp`
    - `src/main.cpp` (initial loading)
    - `assets/definitions/*.yml`
  description:
    Developers should be able to tweak `tiles.yml` and `env_effects.yml` without restarting the game. Implement a hot-reload command (key or UI button) that re-parses definitions and updates relevant runtime structures.
  steps:
    1. Add a `DefinitionManager::Reload()` wrapper that re-parses the YAML files.
    2. Ensure existing tiles keep their tile IDs; only definition-derived properties (sprites, component defaults) change.
    3. Add a safe way to remap existing Tile instances to new TileDefs (or mark for rebuild if incompatible).
    4. Provide a debug key (e.g., F5) to trigger reload; log clearly on success/failure.
  acceptance:
    - Pressing the reload key updates sprites and changed defaults without crashing.

- id: TASK-004
  title: Add unit-test harness and a couple of smoke tests
  priority: P1
  difficulty: Medium
  files:
    - `CMakeLists.txt` (add FetchContent for Catch2 or GoogleTest)
    - `tests/test_tile.cpp`
    - `tests/test_action.cpp`
  description:
    The repo has no formal tests. Add a minimal CMake test target that builds unit tests and runs them during CI.
  steps:
    1. Add Catch2 via `FetchContent` in `CMakeLists.txt` under an optional `BUILD_TESTS` option.
    2. Create tests for `Tile::CreateTile()` (component add/remove) and `MoveAction::Update()` behavior.
    3. Wire `ctest` integration so `make test` runs the suite.
  acceptance:
    - `cmake -B build -S . -DBUILD_TESTS=ON && cmake --build build -- -j && ctest --test-dir build` runs tests and they pass locally.

- id: TASK-005
  title: Add CI workflow to build with Nix and run the game build
  priority: P1
  difficulty: Easy/Medium
  files:
    - `.github/workflows/build.yml`
    - `shell.nix` (already present)
    - `CMakeLists.txt`
  description:
    Add a GitHub Actions workflow that runs `nix-shell`, builds via CMake, and reports build status. Optionally run tests if present.
  steps:
    1. Create a `build.yml` with steps: checkout, install Nix (or use a Nix-enabled runner), run `nix-shell --run "cmake -B build && cmake --build build"`.
    2. Optionally add a `make test` step if tests exist.
  acceptance:
    - Workflow completes successfully on main branch for a basic build.

- id: TASK-006
  title: Improve fixed-timestep threading safety and profiling hooks
  priority: P2
  difficulty: Medium
  files:
    - `src/fixed_update.cpp`
    - `src/update.cpp` / `src/update.hpp`
  description:
    The simulation runs in a background thread with a mutex/condition variable. Add profiling hooks and double-check lock scopes to prevent deadlocks and improve clarity.
  steps:
    1. Review `updateMutex` usage and ensure per-subsystem locking if needed.
    2. Add lightweight scoped timers (e.g., microsecond counters) around `HandleCrewActions()` and `UpdatePowerGrids()` and log occasional samples.
    3. Run a local profiling session to spot hot loops.
  acceptance:
    - No deadlocks observed; logs show per-subsystem times under typical world sizes.

---

Notes & conventions (short)
- Ownership: use `std::shared_ptr` for primary ownership; `std::weak_ptr` for back-references. Many structs use `enable_shared_from_this`.
- Enums: magic_enum is used for conversions and flags (`magic_enum::enum_flags_name`, `EnumToName<>`). See `component.hpp`/`tile.hpp`.
- Data-driven: `assets/definitions/tiles.yml` contains tile component lists and sliced sprite rules. When adding tile visuals, update this file and ensure sprites reference coordinates in `assets/tilesets/*.png`.
- Fixed timestep: main simulation loop uses `FIXED_DELTA_TIME` and `fixed_update.cpp` performs deterministic updates.
- Logging: use `LogMessage(LogLevel::..., "...")` for debug output instead of printing directly.

Workflow for agents
1. Pick a P0 task and create a branch.
2. Make small, incremental edits and push frequently.
3. Run `nix-shell` and build locally (`cmake -B build && cmake --build build`).
4. Run manual smoke tests (launch `./build/celestium` or run unit tests if present).
5. Open a PR and reference the task ID; include screenshots if UI/visual changes were made.

Feedback and maintenance
- Add or edit tasks here as the backlog evolves. Keep tasks small and focused.
- When a task finishes, move it to a `Done` section with PR link and short notes about what changed.
