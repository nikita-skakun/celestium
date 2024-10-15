#pragma once
#include <raylib.h>

constexpr double FIXED_DELTA_TIME = 1. / 45.;

constexpr float MIN_ZOOM = .25f;
constexpr float MAX_ZOOM = 5.f;
constexpr float ZOOM_SPEED = .1f;

constexpr int DEFAULT_FONT_SIZE = 20;
constexpr float DEFAULT_PADDING = 10.f;
constexpr Color UI_TEXT_COLOR = LIGHTGRAY;

constexpr float TILE_SIZE = 32.f;
constexpr float TILE_OXYGEN_MAX = 100.f;
constexpr Color TILE_SELECTION_TINT = Color(255, 128, 255, 255);

constexpr Color GRID_COLOR = Color(0, 0, 0, 50);

constexpr float OXYGEN_DIFFUSION_RATE = 10.f;

constexpr float POWER_CONNECTION_WIDTH = 2.f;
constexpr Color POWER_CONNECTION_COLOR = Color(253, 249, 0, 77);

constexpr float DRAG_THRESHOLD = .25f;
constexpr float OUTLINE_SIZE = 1.f;
constexpr Color OUTLINE_COLOR = BLACK;

constexpr float CREW_RADIUS = 12.f;
constexpr float CREW_MOVE_SPEED = 2.f;
constexpr float CREW_OXYGEN_MAX = 100.f;
constexpr float CREW_OXYGEN_USE = 10.f;
constexpr float CREW_OXYGEN_REFILL = 50.0f;
constexpr float CREW_HEALTH_MAX = 10.f;