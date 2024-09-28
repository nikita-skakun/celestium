#pragma once
#include "station.h"
#include <queue>
#include <unordered_set>

std::deque<Vector2Int> AStar(const Vector2Int &start, const Vector2Int &end, std::shared_ptr<Station> station);
bool DoesPathHaveObstacles(const std::deque<Vector2Int> &path, std::shared_ptr<Station> station, bool canPathInSpace);