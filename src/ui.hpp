#pragma once
#include "camera.hpp"
#include "crew.hpp"
#include "game_state.hpp"
#include "ui_manager.hpp"

void DrawTileGrid(const PlayerCam &camera);
void DrawPath(const std::deque<Vector2Int> &path, const Vector2 &startPos, const PlayerCam &camera);
void DrawStationTiles(std::shared_ptr<Station> station, const PlayerCam &camera);
void DrawStationOverlays(std::shared_ptr<Station> station, const PlayerCam &camera);
void DrawTileOutline(std::shared_ptr<Tile> tile, const PlayerCam &camera);
void DrawEnvironmentalHazards(std::shared_ptr<Station> station, const PlayerCam &camera);
void DrawCrew(double timeSinceFixedUpdate, const std::vector<Crew> &crewList, const PlayerCam &camera);
void DrawDragSelectBox(const PlayerCam &camera);
void DrawFpsCounter(float deltaTime, float padding = DEFAULT_PADDING, int fontSize = DEFAULT_FONT_SIZE);
void DrawTooltip(const std::string &tooltip, const Vector2 &pos, float padding = DEFAULT_PADDING, int fontSize = DEFAULT_FONT_SIZE);
void DrawMainTooltip(const std::vector<Crew> &crewList, const PlayerCam &camera, std::shared_ptr<Station> station);