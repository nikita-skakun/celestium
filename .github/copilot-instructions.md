# Celestium AI Coding Guidelines

## Project Overview
Celestium is a 2D space station simulation game built in C++23 using raylib. It features procedural generation, crew AI with A* pathfinding, real-time air diffusion simulation, power management, and environmental hazards. The architecture uses a component-based system for tiles, with data-driven definitions loaded from YAML files.

## Core Architecture

### Component System
Tiles are composed of components that define their behavior:
- **WalkableComponent**: Allows crew movement
- **SolidComponent**: Blocks vision/pathing
- **PowerConnectorComponent**: Connects to power grids
- **OxygenComponent**: Stores oxygen for diffusion
- **DoorComponent**: Dynamic air flow control
- **BatteryComponent**: Power storage
- **DurabilityComponent**: Health/damage system

Example component access:
```cpp
if (auto oxygenComp = tile->GetComponent<OxygenComponent>()) {
    float level = oxygenComp->GetOxygenLevel();
}
```

### Game State Management
- **GameManager**: Singleton handling global state, crew lists, station reference
- State transitions: NONE → MAIN_MENU → GAME_SIM
- Fixed timestep updates (60 FPS) for simulation consistency
- Multithreaded fixed updates for performance

### Data Flow
1. YAML definitions loaded at startup (`assets/definitions/tiles.yml`, `env_effects.yml`)
2. Tiles instantiated with components based on definitions
3. Simulation updates: crew actions → environment → power → tiles
4. Rendering: station tiles → crew → UI overlays

## Development Workflow

### Key Directories
- `src/`: Core game logic (.cpp/.hpp pairs)
- `assets/definitions/`: YAML configuration files
- `assets/tilesets/`: Sprite sheets
- `assets/audio/`: Sound effects

## Coding Patterns

### Memory Management
- Use `std::shared_ptr` for primary ownership
- `std::weak_ptr` for back-references (e.g., tile to station)
- `std::enable_shared_from_this` for self-referencing objects

### Enums and Constants
- Enum classes with `magic_enum` for string conversion
- Constants defined in `const.hpp`
- Use `EnumToName<Type>()` for debug/logging

### Pathfinding and Movement
- A* algorithm implemented in `astar.cpp`
- Crew actions queued as `std::deque<std::shared_ptr<Action>>`
- Actions: MOVE, EXTINGUISH, REPAIR

### Rendering
- raylib-based with custom sprite system
- Tiles rendered with neighbor-aware sprite conditions
- UI managed through `UiManager` with element hierarchy

### Error Handling
- Asserts for development (`assert()`)
- Logging via `LogMessage()` with levels (INFO, WARN, ERROR)
- Graceful degradation for missing assets

## Common Tasks

### Adding New Tile Types
1. Define in `assets/definitions/tiles.yml` with components
2. Add sprite conditions for neighbor rendering
3. Implement component logic if new component type needed

### Implementing Crew Actions
1. Create new `Action` subclass in `action.hpp/.cpp`
2. Add to `Action::Type` enum
3. Implement `Update()` method with progress tracking
4. Handle in `AssignCrewActions()` and `HandleCrewActions()`

### Modifying Simulation
- Fixed updates in `fixed_update.cpp` for physics/simulation
- Variable updates in `main.cpp` for rendering/input
- Use `FIXED_DELTA_TIME` for time-based calculations

## Dependencies
- **raylib**: Graphics, input, audio
- **rapidyaml**: Configuration parsing
- **magic_enum**: Enum utilities
- **RtAudio/Opus**: Audio playback
- **termcolor**: Console output

## Testing and Debugging
- Build with `CMAKE_BUILD_TYPE=Debug` for assertions
- Use `LogMessage()` for debug output
- Visual debugging: crew selection, tile overlays, FPS counter
- No formal test suite; manual testing required

## File Organization
- Headers (.hpp) declare interfaces, sources (.cpp) implement
- Forward declarations in headers to minimize includes
- `utils.hpp` for common types (Vector2Int, Direction)
- `def_manager.hpp` handles YAML loading and caching</content>
<parameter name="filePath">/home/user/Projects/celestium/.github/copilot-instructions.md