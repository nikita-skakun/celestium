#pragma once
#include "station.h"
#include <queue>
#include <unordered_set>

std::queue<Vector2> AStar(const Vector2 &startRaw, const Vector2 &endRaw, std::shared_ptr<Station> station);
bool DoesPathHaveObstacles(const std::queue<Vector2> &path, std::shared_ptr<Station> station, bool canPathInSpace);