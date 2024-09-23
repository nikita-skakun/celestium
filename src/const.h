#pragma once
#include <raylib.h>

const float MIN_ZOOM = .25f;
const float MAX_ZOOM = 5.f;
const float ZOOM_SPEED = .1f;

const float TILE_SIZE = 32.f;
const float TILE_OXYGEN_MAX = 100.f;

const float OXYGEN_DIFFUSION_RATE = 10.f;
const float OXYGEN_PRODUCTION_RATE = 100.f;

const float DRAG_THRESHOLD = .25f;
const float OUTLINE_SIZE = 1.f;
const Color OUTLINE_COLOR = BLACK;

const float CREW_RADIUS = 12.f;
const float CREW_MOVE_SPEED = 2.f;
const float CREW_OXYGEN_MAX = 100.f;
const float CREW_OXYGEN_USE = 20.f;
const float CREW_OXYGEN_REFILL = 40.0f;