#pragma once
#include "utils.hpp"
#include <deque>

struct Station;

using HeuristicFunction = std::function<float(const Vector2Int &, const Vector2Int &)>;

std::deque<Vector2Int> AStar(const Vector2Int &start, const Vector2Int &end, const HeuristicFunction &heuristic = Vector2IntDistanceSq);
bool DoesPathHaveObstacles(const std::deque<Vector2Int> &path);