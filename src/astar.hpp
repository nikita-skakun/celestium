#pragma once
#include "station.hpp"
#include <queue>
#include <unordered_set>

using HeuristicFunction = std::function<float(const Vector2Int &, const Vector2Int &)>;

std::deque<Vector2Int> AStar(const Vector2Int &start, const Vector2Int &end, std::shared_ptr<Station> station, const HeuristicFunction &heuristic = Vector2IntDistanceSq);
bool DoesPathHaveObstacles(const std::deque<Vector2Int> &path, std::shared_ptr<Station> station, bool canPathInSpace);