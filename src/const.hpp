#pragma once
#include <raylib.h>

constexpr float FIXED_DELTA_TIME = 1.0f / 30.0f;

constexpr float MIN_ZOOM = .25f;
constexpr float MAX_ZOOM = 5.f;
constexpr float ZOOM_SPEED = .1f;

constexpr int DEFAULT_FONT_SIZE = 20;

constexpr float TILE_SIZE = 32.f;
constexpr float TILE_OXYGEN_MAX = 100.f;

constexpr float OXYGEN_DIFFUSION_RATE = 10.f;
constexpr float OXYGEN_PRODUCTION_RATE = 1000.f;

constexpr float BATTERY_CHARGE_MAX = 200.f;

constexpr float DRAG_THRESHOLD = .25f;
constexpr float OUTLINE_SIZE = 1.f;
constexpr Color OUTLINE_COLOR = BLACK;

constexpr float POWER_CONNECTION_WIDTH = 2.f;
constexpr Color POWER_CONNECTION_COLOR = Color(253, 249, 0, 77);

constexpr float CREW_RADIUS = 12.f;
constexpr float CREW_MOVE_SPEED = 2.f;
constexpr float CREW_OXYGEN_MAX = 100.f;
constexpr float CREW_OXYGEN_USE = 100.f;
constexpr float CREW_OXYGEN_REFILL = 200.0f;