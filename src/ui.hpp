#pragma once
#include "camera.hpp"
#include "crew.hpp"

void DrawTileGrid(const PlayerCam &camera);
void DrawPath(const std::deque<Vector2Int> &path, const Vector2 &startPos, const PlayerCam &camera);
void DrawStation(std::shared_ptr<Station> station, const Texture2D &tileset, const PlayerCam &camera);
void DrawCrew(double timeSinceFixedUpdate, const std::vector<Crew> &crewList, const PlayerCam &camera);
void DrawDragSelectBox(const PlayerCam &camera);
void DrawFpsCounter(float deltaTime, int padding, int fontSize = DEFAULT_FONT_SIZE, const Font &font = GetFontDefault());
void DrawTooltip(const std::string &tooltip, const Vector2 &pos, float padding = 10.f, int fontSize = DEFAULT_FONT_SIZE, const Font &font = GetFontDefault());
void DrawMainTooltip(const std::vector<Crew> &crewList, const PlayerCam &camera, std::shared_ptr<Station> station, const Font &font = GetFontDefault());