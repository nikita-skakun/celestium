#pragma once
#include "utils.hpp"

struct PlannedTask
{
    Vector2Int position;
    std::string tileId;
    bool isBuild;
    float progress = 0.0f;
};