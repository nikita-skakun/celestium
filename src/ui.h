#pragma once
#include "camera.h"
#include "crew.h"
#include <queue>

void DrawTileGrid(const PlayerCam &camera);
void DrawPath(const std::queue<Vector2Int> &path, const Vector2 &startPos, const PlayerCam &camera);
void DrawDragSelectBox(const PlayerCam &camera);
void DrawFpsCounter(int textSize, int padding, float deltaTime);
void DrawTooltip(const std::string &tooltip, const Vector2 &pos, float padding = 10.f, int fontSize = DEFAULT_FONT_SIZE, const Font &font = GetFontDefault());
void DrawMainTooltip(const std::vector<Crew> &crewList, const PlayerCam &camera, std::shared_ptr<Station> station, const Font &font = GetFontDefault());