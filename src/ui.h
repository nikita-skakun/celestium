#pragma once
#include "camera.h"
#include <queue>

void DrawTileGrid(const PlayerCam &camera);
void DrawFpsCounter(int textSize, int padding, float deltaTime);
void DrawTooltip(const std::string &tooltip, const Vector2 &pos, int fontSize = 20, float padding = 10.f);
void DrawPath(const std::queue<Vector2Int> &path, const Vector2 &startPos, const PlayerCam &camera);