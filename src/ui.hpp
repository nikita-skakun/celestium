#pragma once
#include "utils.hpp"
#include <deque>

void DrawTileGrid();
void DrawPath(const std::deque<Vector2Int> &path, const Vector2 &startPos);
void DrawStationTiles();
void DrawStationOverlays();
void DrawEnvironmentalEffects();
void DrawCrew();
void DrawCrewActionProgress();
void DrawDragSelectBox();
void DrawFpsCounter();
void DrawResourceUI();
void DrawTooltip(const std::string &tooltip, const Vector2 &pos, float padding = DEFAULT_PADDING, int fontSize = DEFAULT_FONT_SIZE);
void DrawMainTooltip();
void DrawBuildUi();
void DrawPlannedTasks();
void ClearRenderSystems();
void CreateStarfield();
void DrawStarfieldBackground();
void ClearStarfield();