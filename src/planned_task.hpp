#pragma once
#include "utils.hpp"

struct Tile;

struct PlannedTask
{
    Vector2Int position;
    std::string tileId;
    bool isBuild;
    float progress;
    mutable std::shared_ptr<Tile> previewTile = nullptr;

    PlannedTask(const Vector2Int &position, const std::string &tileId, bool isBuild, float progress = 0.f)
        : position(position), tileId(tileId), isBuild(isBuild), progress(progress) {}
};