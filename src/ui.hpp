#pragma once
#include "crew.hpp"
#include "ui_manager.hpp"

void DrawTileGrid();
void DrawPath(const std::deque<Vector2Int> &path, const Vector2 &startPos);
void DrawStationTiles(std::shared_ptr<Station> station);
void DrawStationOverlays(std::shared_ptr<Station> station);
void DrawTileOutline(std::shared_ptr<Tile> tile, Color color);
void DrawEnvironmentalEffects(std::shared_ptr<Station> station);
void DrawCrew(double timeSinceFixedUpdate, const std::vector<Crew> &crewList);
void DrawCrewTaskProgress(const std::vector<Crew> &crewList);
void DrawDragSelectBox();
void DrawFpsCounter(float deltaTime, float padding = DEFAULT_PADDING, int fontSize = DEFAULT_FONT_SIZE);
void DrawTooltip(const std::string &tooltip, const Vector2 &pos, float padding = DEFAULT_PADDING, int fontSize = DEFAULT_FONT_SIZE);
void DrawMainTooltip(const std::vector<Crew> &crewList, std::shared_ptr<Station> station);
void DrawBuildUi(std::shared_ptr<Station> station);