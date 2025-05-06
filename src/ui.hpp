#pragma once
#include "utils.hpp"
#include <deque>

struct Crew;
struct Station;
struct Tile;

void DrawTileGrid();
void DrawPath(const std::deque<Vector2Int> &path, const Vector2 &startPos);
void DrawStationTiles();
void DrawStationOverlays();
void DrawTileOutline(const std::shared_ptr<Tile> &tile, Color color);
void DrawEnvironmentalEffects();
void DrawCrew();
void DrawCrewActionProgress();
void DrawDragSelectBox();
void DrawFpsCounter();
void DrawTooltip(const std::string &tooltip, const Vector2 &pos, float padding = DEFAULT_PADDING, int fontSize = DEFAULT_FONT_SIZE);
void DrawMainTooltip();
void DrawBuildUi();