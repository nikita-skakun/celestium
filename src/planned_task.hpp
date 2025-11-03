#pragma once
#include "utils.hpp"

struct PlannedTask
{
    Vector2Int position;
    std::string tileId;
    bool isBuild;
    float progress;

    PlannedTask(const Vector2Int &position, const std::string &tileId, bool isBuild, float progress = 0.f)
        : position(position), tileId(tileId), isBuild(isBuild), progress(progress) {}
};